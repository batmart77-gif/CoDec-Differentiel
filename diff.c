#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include "image.h"
#include "diff.h"
#include "vlc.h"
#include "normalize.h"

/* ===================== */
/*  image -> diff_image  */
/* ===================== */

static diff_image* image_to_diff(const image *img) {
    if (!img || !img->pixmap) return NULL;

    diff_image *diff = malloc(sizeof(diff_image));
    if (!diff) return NULL;

    diff->width  = img->width;
    diff->height = img->height;
    diff->layer  = img->layer;

    size_t size = (size_t)img->width * img->height * img->layer;
    diff->data = malloc(size * sizeof(int16_t));
    if (!diff->data) {
        free(diff);
        return NULL;
    }

    // On crée un pixmap temporaire pour simuler la reconstruction du décodeur
    // Cela permet de calculer la différence par rapport à ce que le décodeur "voit"
    unsigned char *reconstructed = malloc(size);
    if (!reconstructed) {
        free(diff->data);
        free(diff);
        return NULL;
    }

    // 1. Gestion des premiers pixels (First Pixels)
    for (size_t c = 0; c < img->layer; ++c) {
        // On divise par 2 pour le stockage
        int16_t val = (int16_t)(img->pixmap[c] / 2);
        diff->data[c] = val;

        // Simulation du décodeur : il fera val * 2
        int rec_val = val * 2;
        if (rec_val > 255) rec_val = 255;
        reconstructed[c] = (unsigned char)rec_val;
    }

    // 2. Boucle avec prédiction (Feedback Loop)
    for (size_t i = img->layer; i < size; ++i) {
        int curr = (int)img->pixmap[i];
        // On calcule la différence par rapport au pixel RECONSTRUIT précédent
        int prev = (int)reconstructed[i - img->layer]; 
        
        int d = curr - prev;
        int16_t encoded_diff;

        // Division par 2 avec arrondi correct
        if (d >= 0) encoded_diff = (int16_t)(d / 2);
        else        encoded_diff = (int16_t)((d - 1) / 2);

        diff->data[i] = encoded_diff;

        // On simule la reconstruction pour le pixel SUIVANT
        int rec_val = prev + (encoded_diff * 2);
        
        // Clamping (indispensable pour que l'encodeur et le décodeur soient synchrones)
        if (rec_val < 0)   rec_val = 0;
        if (rec_val > 255) rec_val = 255;
        
        reconstructed[i] = (unsigned char)rec_val;
    }

    free(reconstructed);
    return diff;
}



image *diff_to_image(const diff_image *diff) {
    if (!diff || !diff->data)
        return NULL;

    image *img = malloc(sizeof(image));
    if (!img)
        return NULL;

    img->width  = diff->width;
    img->height = diff->height;
    img->layer  = diff->layer;
    img->max_val = 255;
    img->pixmap_size = img->width * img->height * img->layer;

    img->pixmap = malloc(img->pixmap_size);
    if (!img->pixmap) {
        free(img);
        return NULL;
    }

    if (img->layer == 1) {
        img->magic_number[0] = 'P';
        img->magic_number[1] = '5';
    } else {
        img->magic_number[0] = 'P';
        img->magic_number[1] = '6';
    }
    img->magic_number[2] = '\0';

    size_t size = img->pixmap_size;

    for (size_t c = 0; c < img->layer; ++c) {
        int v = (int)diff->data[c];

        if (v < 0)   v = 0;
        if (v > 255) v = 255;

        img->pixmap[c] = (unsigned char)v;
    }

    for (size_t i = img->layer; i < size; ++i) {
        int prev = (int)img->pixmap[i - img->layer];
        int d    = (int)diff->data[i];
        int curr = (prev + d);

        if (curr < 0)   curr = 0;
        if (curr > 255) curr = 255;

        img->pixmap[i] = (unsigned char)curr;
    }

    return img;
}


int pnmtodif(const char *pnm_path, const char *dif_path)
{
    FILE *out = NULL;
    image *img = NULL;
    diff_image *diff = NULL;
    unsigned int *norm_data = NULL;
    unsigned char *vlc_buf = NULL;
    size_t vlc_bytes = 0;
    int ret = 1; // Échec par défaut

    // 1. Paramètres de compression (Ajustables)
    unsigned char q = 4;
    unsigned char my_qbits[4] = {1, 2, 4, 8};

    // 2. Chargement de l'image PNM
    img = malloc(sizeof(image));
    if (!read_image(img, pnm_path)) {
        fprintf(stderr, "pnmtodif: Erreur lecture image %s\n", pnm_path);
        free(img);
        return 1;
    }

    // 3. Transformation en image différentielle
    diff = image_to_diff(img);
    if (!diff) goto cleanup;

    // 4. Normalisation
    size_t total_count = (size_t)diff->width * diff->height * diff->layer;
    norm_data = normalize_diff(diff->data, total_count);
    if (!norm_data) goto cleanup;

    // 5. Encodage VLC (On saute les 'layer' premiers coefficients)
    size_t rest = total_count - diff->layer;
    unsigned int *to_encode = norm_data + diff->layer;

    if (!vlc_encode(to_encode, rest, &vlc_buf, &vlc_bytes, my_qbits)) {
        fprintf(stderr, "pnmtodif: Erreur encodage VLC\n");
        goto cleanup;
    }

    // 6. Ouverture et écriture du fichier DIF
    out = fopen(dif_path, "wb");
    if (!out) {
        perror("pnmtodif: fopen output");
        goto cleanup;
    }

    // Header (11 octets)
    unsigned short magic = (diff->layer == 1) ? 0xD1FF : 0xD3FF;
    unsigned short w = diff->width;
    unsigned short h = diff->height;

    fwrite(&magic, 2, 1, out);
    fwrite(&w, 2, 1, out);
    fwrite(&h, 2, 1, out);
    fwrite(&q, 1, 1, out);
    fwrite(my_qbits, 1, 4, out);

    // Écriture des First Pixels
    for (size_t c = 0; c < diff->layer; ++c) {
        unsigned char first = (unsigned char)(diff->data[c]);
        fwrite(&first, 1, 1, out);
    }

    // Écriture du flux VLC
    if (fwrite(vlc_buf, 1, vlc_bytes, out) != vlc_bytes) {
        fprintf(stderr, "pnmtodif: Erreur écriture data\n");
        goto cleanup;
    }

    ret = 0; // Succès !
    printf("Compression réussie : %s -> %s (%zu octets vlc)\n", pnm_path, dif_path, vlc_bytes);

cleanup:
    if (out) fclose(out);
    if (vlc_buf) free(vlc_buf);
    if (norm_data) free(norm_data);
    if (diff) {
        if (diff->data) free(diff->data);
        free(diff);
    }
    if (img) {
        free_image(img);
        free(img);
    }
    return ret;
}

int diftopnm(const char *dif_path, const char *pnm_path)
{
    FILE *in = NULL;
    image *img = NULL;
    diff_image *diff = NULL;
    unsigned int *norm = NULL;
    unsigned char *vlc_buf = NULL;
    int ret = 1; /* échec par défaut */

    /* ---------- 1. OUVERTURE FICHIER DIF ---------- */
    in = fopen(dif_path, "rb");
    if (!in) {
        perror("diftopnm: fopen input");
        goto cleanup;
    }

    /* ---------- 2. LECTURE EN-TÊTE ---------- */
    unsigned short magic = 0;
    unsigned short w = 0, h = 0;
    unsigned char q = 0;
    unsigned char qbits[4];

    if (fread(&magic, 2, 1, in) != 1) goto cleanup;
    if (fread(&w, 2, 1, in) != 1) goto cleanup;
    if (fread(&h, 2, 1, in) != 1) goto cleanup;

    printf("\n--- DEBUG HEADER ---\n");
    printf("Magic lu : 0x%04X\n", magic);
    printf("Largeur (W) lue : %u\n", w);
    printf("Hauteur (H) lue : %u\n", h);
    printf("--------------------\n");

    if (fread(&q, 1, 1, in) != 1) goto cleanup;
    printf("Q lu : %u\n", q);
    if (fread(qbits, 1, 4, in) != 4) goto cleanup;
    printf("Qbits du fichier : %u, %u, %u, %u\n", qbits[0], qbits[1], qbits[2], qbits[3]);

    /* Vérif rapide du magic pour déterminer layer (P5/P6) */
    size_t layer = 0;
    if (magic == 0xD1FF) {
        layer = 1; /* PGM */
    } else if (magic == 0xD3FF) {
        layer = 3; /* PPM */
    } else {
        fprintf(stderr, "diftopnm: magic number DIF invalide (0x%04X)\n", magic);
        goto cleanup;
    }

    /* ---------- 3. ALLOCATION diff_image ET LECTURE FIRST PIXELS ---------- */
    diff = malloc(sizeof(*diff));
    if (!diff) goto cleanup;

    diff->width  = w;
    diff->height = h;
    diff->layer  = layer;

    size_t total = (size_t)w * (size_t)h * layer;
    diff->data = malloc(total * sizeof(int16_t));
    if (!diff->data) goto cleanup;

    /* first pixels (layer premiers coefficients, stockés comme diff->data[c]*2 à l'encode) */
    for (size_t c = 0; c < layer; ++c) {
        unsigned char first = 0;
        if (fread(&first, 1, 1, in) != 1) goto cleanup;
        diff->data[c] = (int16_t)first * 2;
    }

    /* ---------- 4. LECTURE BUFFER VLC ---------- */
    /* On lit tout le reste du fichier en mémoire */
    if (fseek(in, 0, SEEK_END) != 0) goto cleanup;
    long end_pos = ftell(in);
    if (end_pos < 0) goto cleanup;
    if (fseek(in, 2 + 2 + 2 + 1 + 4 + (long)layer, SEEK_SET) != 0) goto cleanup;

    long data_pos = ftell(in);
    if (data_pos < 0) goto cleanup;
    size_t vlc_bytes = (size_t)(end_pos - data_pos);

    vlc_buf = malloc(vlc_bytes);
    if (!vlc_buf) goto cleanup;
    if (fread(vlc_buf, 1, vlc_bytes, in) != vlc_bytes) goto cleanup;

    /* ---------- 5. VLC DECODE + DÉNORMALISATION ---------- */
    /* On n’a encodé que total - layer coefficients différentiels */
    size_t rest = total - layer;

    norm = malloc(rest * sizeof(unsigned int));
    if (!norm) goto cleanup;

    if (vlc_decode(vlc_buf, vlc_bytes, norm, rest, qbits) == 0) {
        fprintf(stderr, "diftopnm: vlc_decode a échoué\n");
        goto cleanup;
    }

    /* Dénormalisation dans diff->data[layer..] */
    // On écrit le résultat directement dans diff->data en sautant les 'layer' premiers
    denormalize_diff(norm, rest, diff->data + layer);

    /* ---------- 6. DIFF -> IMAGE ---------- */
    img = diff_to_image(diff);
    if (!img) {
        fprintf(stderr, "diftopnm: diff_to_image a échoué\n");
        goto cleanup;
    }

    /* ---------- 7. ÉCRITURE PNM ---------- */
    if (!write_image(img, pnm_path)) {
        fprintf(stderr, "diftopnm: write_image a échoué\n");
        goto cleanup;
    }

    ret = 0; /* succès */

    cleanup:
    if (in) fclose(in);
    free(vlc_buf);
    free(norm);
    if (diff) {
        free(diff->data);
        free(diff);
    }
    if (img) {
        free_image(img);
        free(img);
    }
    return ret;
}