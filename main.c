#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "image.h"
#include "diff.h"

int main(int argc, char *argv[]) {
    int opt;
    char *image_path = NULL;

    image img = {0};

    /*Gestion des options */
    while ((opt = getopt(argc, argv, "i:")) != -1) {
        switch (opt) {
            case 'i':
                image_path = strdup(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -i <image>\n", argv[0]);
                return 1;
        }
    }

    if (image_path == NULL) {
        fprintf(stderr, "Erreur: option -i obligatoire\n");
        return 1;
    }

    /* Lecture image (TP7)*/
    if (!read_image(&img, image_path)) {
        fprintf(stderr, "Erreur: impossible de lire l'image\n");
        goto cleanup;
    }

    printf("Image chargée: %zu x %zu, layers=%zu\n",
           img.width, img.height, img.layer);

    /*Transformation diff */
    if (!image_to_diff(&img)) {
        fprintf(stderr, "Erreur: image_to_diff a échoué\n");
        goto cleanup;
    }

    printf("Transformation différentielle OK\n");

cleanup:
    free_image(&img);
    free(image_path);
    return 0;
}
