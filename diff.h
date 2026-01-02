#ifndef DIFF_H
#define DIFF_H

#include <stdio.h>

typedef struct {
    size_t width;
    size_t height;
    size_t layer;
    signed char *data;   // IMPORTANT : signé
} diff_image;

diff_image* image_to_diff(const image *img);

#endif