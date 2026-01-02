#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include "image.h"
#include "diff.h"

diff_image* image_to_diff(const image *img) {
    if (!img || !img->pixmap) return NULL;

    diff_image *diff = malloc(sizeof(diff_image));
    if (!diff) return NULL;

    diff->width = img->width;
    diff->height = img->height;
    diff->layer = img->layer;

    size_t size = img->width * img->height * img->layer;
    diff->data = malloc(size * sizeof(signed char));
    if (!diff->data) {
        free(diff);
        return NULL;
    }

    /*Premier pixel = copié tel quel*/
    for (size_t c = 0; c < img->layer; c++) {
        diff->data[c] = (signed char)(img->pixmap[c] / 2);
    }

    /*Différences*/
    for (size_t i = img->layer; i < size; i++) {
        int curr = img->pixmap[i] / 2;
        int prev = img->pixmap[i - img->layer] / 2;
        diff->data[i] = (signed char)(curr - prev);
    }

    return diff;
}