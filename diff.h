#ifndef DIFF_H
#define DIFF_H

#include <stdio.h>
#include <stdint.h>
#include "image.h"

typedef struct {
    unsigned short width;
    unsigned short height;
    size_t layer;
    int16_t *data;   // IMPORTANT : signé
} diff_image;

int pnmtodif(const char *pnm_path, const char *dif_path);
image *diff_to_image(const diff_image *diff);
int diftopnm(const char *dif_path, const char *pnm_path);

#endif
