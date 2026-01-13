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

    norm = normalize_diff(diff);
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

    ret = 0;

cleanup:
    if (out) fclose(out);
    free(vlc_buf);
    free(norm);
    if (diff) { free(diff->data); free(diff); }
    free_image(&img);
    return ret;
}

