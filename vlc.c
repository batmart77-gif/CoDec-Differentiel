#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "bitstream.h"
#include "vlc.h"

/* Quantificateur fixe du sujet (4 intervalles) [file:136]
 *
 * I0 : [0,4[    longueur 4  = 2^2  prefixe "0"
 * I1 : [4,20[   longueur 16 = 2^4  prefixe "10"
 * I2 : [20,52[  longueur 32 = 2^5  prefixe "110"
 * I3 : [52,256[ longueur 204 ~ 2^8 prefixe "111"
 */

typedef struct {
    unsigned int binf;   /* borne inf incluse */
    unsigned int bsup;   /* borne sup exclue */
    unsigned int p;      /* nb de bits pour y = x-binf */
    unsigned char prefix;/* bits du préfixe, alignés à gauche dans un octet */
    unsigned int plen;   /* nb de bits significatifs dans prefix */
} QuantInterval;

static const QuantInterval Q[4] = {
    {  0u,   4u, 2u, 0b00000000, 1u }, /* "0"   */
    {  4u,  20u, 4u, 0b10000000, 2u }, /* "10"  */
    { 20u,  52u, 5u, 0b11000000, 3u }, /* "110" */
    { 52u, 256u, 8u, 0b11100000, 3u }  /* "111" */
};

/* Cherche l'intervalle contenant x dans Q, retourne son index (0..3) ou -1 si erreur */
static int find_interval(unsigned int x)
{
    for (int k = 0; k < 4; ++k) {
        if (x >= Q[k].binf && x < Q[k].bsup)
            return k;
    }
    return -1;
}

/* ----------------- ENCODAGE ----------------- */

size_t vlc_encode(const unsigned int *in, size_t n,
                  unsigned char **out_buf, size_t *out_bytes)
{
    if (!in || !out_buf || !out_bytes)
        return 0;

    /* estimation large : au pire 11 bits par valeur => (11*n)/8 octets */
    size_t max_bits  = 11 * n;
    size_t buf_bytes = (max_bits + CHAR_BIT - 1) / CHAR_BIT;
    if (buf_bytes == 0)
        buf_bytes = 1;

    unsigned char *buf = calloc(buf_bytes, 1);
    if (!buf)
        return 0;

    BitStream bs;
    bs.ptr = buf;
    bs.cap = CHAR_BIT;

    size_t total_bits = 0;

    for (size_t i = 0; i < n; ++i) {
        unsigned int x = in[i];
        int idx = find_interval(x);
        if (idx < 0) {
            /* valeur hors [0,255] -> erreur */
            free(buf);
            return 0;
        }

        const QuantInterval *qi = &Q[idx];

        /* a) préfixe Ck */
        unsigned char pref = qi->prefix;
        /* on garde seulement les plen bits de poids fort */
        pref &= (unsigned char)(0xFFu << (CHAR_BIT - qi->plen));
        total_bits += pushbits(pref, qi->plen, &bs);

        /* b) valeur translatée y = x - binf sur p bits */
        unsigned int y = x - qi->binf;
        unsigned char ybyte = (unsigned char)y;
        total_bits += pushbits(ybyte, qi->p, &bs);
    }

    *out_buf   = buf;
    *out_bytes = (total_bits + CHAR_BIT - 1) / CHAR_BIT;
    return total_bits;
}

/* ----------------- DECODAGE ----------------- */

/* essaie de lire un préfixe (1 à 3 bits) et retourne l'intervalle trouvé,
 * en avançant le BitStream ; -1 en cas d'erreur. */
static int read_prefix(BitStream *bs)
{
    unsigned char tmp = 0;
    BitStream save = *bs; /* pour annuler en cas d'échec */

    /* 1er bit : si 0 => intervalle 0 */
    if (pullbits(bs, &tmp, 1) != 1)
        return -1;
    if ((tmp & 0x01u) == 0u)
        return 0;

    /* on a lu '1', il faut lire un 2e bit */
    tmp = 0;
    if (pullbits(bs, &tmp, 1) != 1) {
        *bs = save;
        return -1;
    }

    if ((tmp & 0x01u) == 0u) {
        /* "10" */
        return 1;
    }

    /* on a "11", il faut lire un 3e bit */
    tmp = 0;
    if (pullbits(bs, &tmp, 1) != 1) {
        *bs = save;
        return -1;
    }
    if ((tmp & 0x01u) == 0u) {
        /* "110" */
        return 2;
    }
    /* "111" */
    return 3;
}

size_t vlc_decode(const unsigned char *in_buf, size_t in_bytes,
                  unsigned int *out, size_t n)
{
    if (!in_buf || !out || in_bytes == 0)
        return 0;

    BitStream bs;
    bs.ptr = (unsigned char *)in_buf;
    bs.cap = CHAR_BIT;

    size_t total_bits = 0;
    size_t max_bits   = in_bytes * CHAR_BIT;

    for (size_t i = 0; i < n; ++i) {
        if (total_bits >= max_bits)
            return 0;

        int idx = read_prefix(&bs);
        if (idx < 0)
            return 0;

        const QuantInterval *qi = &Q[idx];

        unsigned char ybyte = 0;
        if (pullbits(&bs, &ybyte, qi->p) != qi->p)
            return 0;

        unsigned int y = (unsigned int)ybyte;
        unsigned int x = qi->binf + y;
        out[i] = x;

        total_bits = (size_t)((bs.ptr - in_buf) * CHAR_BIT + (CHAR_BIT - bs.cap));
    }

    return total_bits;
}
