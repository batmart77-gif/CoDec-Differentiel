#ifndef DIFF_H
#define DIFF_H

#include <stdio.h>
#include "image.h"

typedef struct {
    size_t width;
    size_t height;
    size_t layer;
    signed char *data;   // IMPORTANT : signé
} diff_image;

/* Encodeur : image PNM -> diff en mémoire */
diff_image* image_to_diff(const image *img);

/* Décodeur : diff en mémoire -> image PNM */
image* diff_to_image(const diff_image *diff);

/* Décodeur fichier : DIF -> PNM (squelettes, à compléter quand tu auras le format DIF) */
int diftopnm(const char *difinput, const char *pnmoutput);

#endif
