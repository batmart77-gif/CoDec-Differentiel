#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include "image.h"
#include "diff.h"

/* ===================== */
/*  image -> diff_image  */
/* ===================== */

diff_image* image_to_diff(const image *img) {
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


// static int read_dif(const char *path, diff_image *diff);

// int diftopnm(const char *difinput, const char *pnmoutput)
// {
//     diff_image diff = {0};
//     image *img = NULL;
//     int ret = 1; 

//     if (!read_dif(difinput, &diff)) {
//         goto cleanup;
//     }

//     img = diff_to_image(&diff);
//     if (!img) {
//         goto cleanup;
//     }

//     if (!write_image(img, pnmoutput)) {
//         goto cleanup;
//     }

//     ret = 0; 

// cleanup:
//     if (diff.data)
//         free(diff.data);
//     if (img) {
//         free_image(img);
//         free(img);
//     }
//     return ret;
// }

// static int read_dif(const char *path, diff_image *diff)
// {
//     (void)path;
//     (void)diff;
//     return 0;
// }
