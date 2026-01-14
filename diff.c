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

    size_t size = img->width * img->height * img->layer;
    diff->data = malloc(size * sizeof(int16_t));
    if (!diff->data) {
        free(diff);
        return NULL;
    }

    for (size_t c = 0; c < img->layer; ++c) {
        diff->data[c] = (int16_t)(img->pixmap[c] / 2);
    }

    for (size_t i = img->layer; i < size; ++i) {
        int curr = img->pixmap[i] / 2;
        int prev = img->pixmap[i - img->layer] / 2;
        diff->data[i] = (int16_t)(curr - prev);
    }

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
        int v = diff->data[c];

        if (v < 0)   v = 0;
        if (v > 127) v = 127;

        img->pixmap[c] = (unsigned char)(v*2);
    }

    for (size_t i = img->layer; i < size; ++i) {
        int prev = img->pixmap[i - img->layer] / 2;
        int d    = (int)diff->data[i];
        int curr = (prev + d);

        if (curr < 0)   curr = 0;
        if (curr > 127) curr = 127;

        img->pixmap[i] = (unsigned char)(curr*2);
    }

    return img;
}


int pnmtodif(const char *pnm_path, const char *dif_path) {
    image img = {0};
    diff_image *diff = NULL;
    unsigned int *norm = NULL;
    unsigned char *vlc_buf = NULL;
    size_t vlc_bytes = 0;
    FILE *out = NULL;
    int ret = 1;

    /* 1. Lecture image */
    if (!read_image(&img, pnm_path)) goto cleanup;

    /* 2. Différentiel */
    diff = image_to_diff(&img);
    if (!diff) goto cleanup;

    size_t total = img.width * img.height * img.layer;
    size_t rest  = total - img.layer;

    /* 3. Normalisation (SAUF first pixel) */
    norm = malloc(rest * sizeof(unsigned int));
    if (!norm) goto cleanup;

    // On envoie diff->data + img.layer pour ne normaliser QUE les différences
    norm = normalize_diff(diff->data + img.layer, rest);
    if (!norm) {
        fprintf(stderr, "bouuu t trop nul");
    }

    /* 4. VLC */
    if (!vlc_encode(norm, rest, &vlc_buf, &vlc_bytes)) goto cleanup;

    /* 5. Ouverture fichier */
    out = fopen(dif_path, "wb");
    if (!out) goto cleanup;

    /* ---------- EN-TÊTE ---------- */

    unsigned short magic = (img.layer == 1) ? 0xD1FF : 0xD3FF;
    unsigned short w = img.width;
    unsigned short h = img.height;

    fwrite(&magic, 2, 1, out);
    fwrite(&w, 2, 1, out);
    fwrite(&h, 2, 1, out);

    unsigned char q = 4;
    unsigned char qbits[4] = {1,2,4,8};

    fwrite(&q, 1, 1, out);
    fwrite(qbits, 1, 4, out);

    /* ---------- FIRST PIXEL ---------- */
    for (size_t c = 0; c < img.layer; c++) {
        unsigned char first = (unsigned char)(diff->data[c] * 2);
        fwrite(&first, 1, 1, out);
    }

    /* ---------- BUFFER VLC ---------- */
    fwrite(vlc_buf, 1, vlc_bytes, out);

    size_t brute = (size_t)img.width * img.height * img.layer;
    
    // Le fichier DIF fait : 2(magic) + 2(w) + 2(h) + 1(q) + 4(qbits) + 1*layer(first pix) + vlc_bytes
    size_t comp = 11 + img.layer + vlc_bytes; 
    
    double ratio = (double)comp / brute;
    double gain = (1.0 - ratio) * 100.0;

    printf("\n--- Statistiques de compression ---\n");
    printf("Taille originale     : %zu octets\n", brute);
    printf("Taille compressée    : %zu octets\n", comp);
    printf("Taux de compression  : %.2f%%\n", ratio*100);
    printf("Espace gagné (Gain)  : %.2f%%\n", gain);
    printf("------------------------------------\n");

    ret = 0;

cleanup:
    if (out) fclose(out);
    free(vlc_buf);
    free(norm);
    if (diff) { free(diff->data); free(diff); }
    free_image(&img);
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
    uint16_t magic = 0;
    uint16_t w = 0, h = 0;
    unsigned char q = 0;
    unsigned char qbits[4];

    if (fread(&magic, 2, 1, in) != 1) goto cleanup;
    if (fread(&w, 2, 1, in) != 1) goto cleanup;
    if (fread(&h, 2, 1, in) != 1) goto cleanup;
    if (fread(&q, 1, 1, in) != 1) goto cleanup;
    if (fread(qbits, 1, 4, in) != 4) goto cleanup;

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
        diff->data[c] = (int16_t)(first / 2);
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

    if (vlc_decode(vlc_buf, vlc_bytes, norm, rest) == 0) {
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