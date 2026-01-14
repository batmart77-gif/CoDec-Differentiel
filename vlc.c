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
    {  0u,   4u, 2u, 0, 1u }, /* "0"   */
    {  4u,  20u, 4u, 2, 2u }, /* "10"  */
    { 20u,  52u, 5u, 6, 3u }, /* "110" */
    { 52u, 256u, 8u, 7, 3u }  /* "111" */
};

/* Cherche l'intervalle contenant x dans Q, retourne son index (0..3) ou -1 si erreur */
static int find_interval(unsigned int x) {
    for (int k = 0; k < 4; ++k) {
        if (x >= Q[k].binf && x < Q[k].bsup)
            return k;
    }
    return -1;
}

/* ----------------- ENCODAGE ----------------- */

size_t vlc_encode(const unsigned int *in, size_t n, unsigned char **out_buf, size_t *out_size) {
    if (!in || n == 0) return 0;

    // 1. Allocation du buffer de sortie (12 bits max par pixel -> 2 octets/pixel c'est large)
    size_t capacity = (size_t)(n * 1.5); 
    *out_buf = calloc(capacity, sizeof(unsigned char)); // calloc met tout à 0 (crucial !)
    if (!*out_buf) return 0;

    BitStream bs;
    bs.ptr = *out_buf;
    bs.cap = 8; // On commence avec un octet vide (8 bits de capacité)

    size_t total_bits = 0;

    for (size_t i = 0; i < n; i++) {
        unsigned int x = in[i];
        
        // 2. Trouver l'intervalle correspondant
        int idx = find_interval(x);
        if (idx == -1) {
            fprintf(stderr, "Erreur: valeur %u hors intervalles VLC\n", x);
            free(*out_buf);
            return 0;
        }
        const QuantInterval *qi = &Q[idx];

        // 3. Écrire le préfixe
        total_bits += pushbits((unsigned char)qi->prefix, qi->plen, &bs);

        // 4. Écrire la valeur y = x - binf
        unsigned int y = x - qi->binf;

        // GESTION SPECIALE si p > 8 bits (car pushbits prend un unsigned char)
        if (qi->p > 8) {
            // On envoie les bits de poids fort (tout sauf le dernier bit)
            total_bits += pushbits((unsigned char)(y >> 1), qi->p - 1, &bs);
            // On envoie le dernier bit
            total_bits += pushbits((unsigned char)(y & 0x01), 1, &bs);
        } else {
            total_bits += pushbits((unsigned char)y, qi->p, &bs);
        }
    }

    // 5. Calculer la taille finale en octets
    // Si on a fini au milieu d'un octet (cap < 8), cet octet compte
    *out_size = (bs.ptr - *out_buf) + (bs.cap < CHAR_BIT ? 1 : 0);

    return total_bits;
}

/* ----------------- DECODAGE ----------------- */

/* essaie de lire un préfixe (1 à 3 bits) et retourne l'intervalle trouvé,
 * en avançant le BitStream ; -1 en cas d'erreur. */
static int read_prefix(BitStream *bs) {
    unsigned char tmp = 0;
    BitStream save = *bs;

    /* 1er bit : si 0 => intervalle 0 */
    if (pullbits(bs, &tmp, 1) != 1)
        return -1;
    if ((tmp & 0x01u) == 0u)
        return 0;

    /* 2e bit : "10" ou "11" */
    tmp = 0;
    if (pullbits(bs, &tmp, 1) != 1) { *bs = save; return -1; }
    if ((tmp & 0x01u) == 0u)
        return 1;

    /* 3e bit : "110" ou "111" */
    tmp = 0;
    if (pullbits(bs, &tmp, 1) != 1) { *bs = save; return -1; }
    if ((tmp & 0x01u) == 0u)
        return 2;

    return 3; /* "111" */
}

size_t vlc_decode(const unsigned char *in_buf, size_t in_bytes, unsigned int *out, size_t n) {
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
