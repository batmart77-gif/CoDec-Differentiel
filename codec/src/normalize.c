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
        
    }
    return norm;
}

void denormalize_diff(const unsigned int *norm, size_t size, int16_t *diff) {
    for (size_t i = 0; i < size; i++) {
        unsigned int v = norm[i];
        int16_t res;

        if (v % 2 == 0) {
            res = (int16_t)(v / 2);
        } else {
            res = (int16_t)-((v + 1) / 2);
        }

        diff[i] = (int16_t)(res * 2); 
    }
}
