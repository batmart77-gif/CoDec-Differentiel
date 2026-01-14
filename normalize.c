#include <stdlib.h>
#include "normalize.h"

unsigned int *normalize_diff(const int16_t *data, size_t size) {
    if (!data || size == 0) return NULL;

    unsigned int *norm = malloc(size * sizeof(unsigned int));
    if (!norm) return NULL;

    for (size_t i = 0; i < size; ++i) {
        int x = data[i];
        if (x >= 0) norm[i] = 2 * x;
        else        norm[i] = -2 * x - 1;
        
        // Suration à 255 : OK pour rester dans I3
        if (norm[i] > 255) norm[i] = 255;
    }
    return norm;
}

void denormalize_diff(const unsigned int *norm, size_t size, int16_t *out_data) {
    for (size_t i = 0; i < size; ++i) {
        unsigned int y = norm[i];
        if (y % 2 == 0)
            out_data[i] = (int16_t)(y / 2);
        else
            out_data[i] = (int16_t)(-(int)(y + 1) / 2);
    }
}
