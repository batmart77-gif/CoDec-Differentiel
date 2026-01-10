#include <stdlib.h>
#include "normalize.h"

unsigned int *normalize_diff(const diff_image *diff) {
    if (!diff || !diff->data) return NULL;

    size_t size = diff->width * diff->height * diff->layer;
    unsigned int *norm = malloc(size * sizeof(unsigned int));
    if (!norm) return NULL;

    for (size_t i = 0; i < size; ++i) {
        int x = diff->data[i];
        if (x >= 0)
            norm[i] = 2 * x;
        else
            norm[i] = -2 * x - 1;
    }

    return norm;
}

void denormalize_diff(const unsigned int *norm, size_t size, diff_image *diff) {
    for (size_t i = 0; i < size; ++i) {
        unsigned int y = norm[i];
        if (y % 2 == 0)
            diff->data[i] = (int16_t)(y / 2);
        else
            diff->data[i] = (int16_t)(-(int)(y + 1) / 2);
    }
}
