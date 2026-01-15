#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "bitstream.h"
#include "vlc.h"

static void write_prefix(BitStream *bs, int idx) {
    switch (idx) {
        case 0:
            pushbits(0, 1, bs); // Valeur 0, 1 bit, vers bs
            break;
        case 1:
            pushbits(2, 2, bs); // Valeur 2 (10 en binaire), 2 bits, vers bs
            break;
        case 2:
            pushbits(6, 3, bs); // Valeur 6 (110 en binaire), 3 bits, vers bs
            break;
        case 3:
            pushbits(7, 3, bs); // Valeur 7 (111 en binaire), 3 bits, vers bs
            break;
    }
}

/* essaie de lire un préfixe (1 à 3 bits) et retourne l'intervalle trouvé,
 * en avançant le BitStream ; -1 en cas d'erreur. */
static int read_prefix(BitStream *bs) {
    unsigned int tmp = 0;
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

/* ----------------- ENCODAGE ----------------- */

int vlc_encode(unsigned int *input, size_t n, unsigned char **out_buf, size_t *out_bytes, unsigned char *qbits) {
    if (!input || n == 0) return 0;

    // 1. Calcul des bornes binf à partir des qbits fournis
    unsigned int binf[4];
    binf[0] = 0;
    binf[1] = (1 << qbits[0]);
    binf[2] = binf[1] + (1 << qbits[1]);
    binf[3] = binf[2] + (1 << qbits[2]);

    // 2. Initialisation du BitStream (on prévoit assez large pour le buffer)
    BitStream bs;
    *out_buf = calloc(n * 4, 1); // Allocation sécurisée
    bs.ptr = *out_buf;
    bs.cap = 8;

    for (size_t i = 0; i < n; ++i) {
        unsigned int x = input[i];
        int idx;
        
        // 3. Choix de l'intervalle
        if (x < binf[1])      idx = 0;
        else if (x < binf[2]) idx = 1;
        else if (x < binf[3]) idx = 2;
        else                  idx = 3;

        // 4. Écriture du préfixe (Huffman)
        // (On suppose que write_prefix écrit 0, 10, 110 ou 111 selon idx)
        write_prefix(&bs, idx);

        // 5. Écriture des bits de données (x - binf)
        unsigned int y = x - binf[idx];
        pushbits(y, (size_t)qbits[idx], &bs);
    }

    // On finalise la taille
    *out_bytes = (bs.ptr - *out_buf) + (bs.cap < 8 ? 1 : 0);
    return 1;
}

/* ----------------- DECODAGE ----------------- */

int vlc_decode(const unsigned char *in_buf, size_t in_bytes, unsigned int *out, size_t n, unsigned char *qbits) {
    if (!in_buf || !out || in_bytes == 0) return 0;

    // On reconstruit les intervalles à la volée selon les qbits du fichier
    // I0: [0, 2^q0 - 1]
    // I1: [2^q0, 2^q0 + 2^q1 - 1] ...
    unsigned int binf[4];
    binf[0] = 0;
    binf[1] = (1 << qbits[0]);
    binf[2] = binf[1] + (1 << qbits[1]);
    binf[3] = binf[2] + (1 << qbits[2]);

    BitStream bs;
    bs.ptr = (unsigned char *)in_buf;
    bs.cap = CHAR_BIT;

    for (size_t i = 0; i < n; ++i) {
        int idx = read_prefix(&bs); // Lit 0, 10, 110 ou 111
        if (idx < 0 || idx > 3) return 0;

        unsigned int p = qbits[idx];
        unsigned int ybyte = 0;
        
        // On récupère les bits de données
        if (pullbits(&bs, &ybyte, p) != p) return 0;

        // Calcul de la valeur finale x = binf + y
        out[i] = binf[idx] + (unsigned int)ybyte;
    }
    return 1; // Succès
}
