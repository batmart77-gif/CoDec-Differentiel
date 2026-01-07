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
    diff_image *diff = NULL;
    image *recon = NULL;


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
    diff = image_to_diff(&img);
    if (!diff) {
        fprintf(stderr, "Erreur: image_to_diff a échoué\n");
        goto cleanup;
    }

    printf("Transformation différentielle OK\n");

    /* diff -> image */
    recon = diff_to_image(diff);
    if (!recon) {
        fprintf(stderr, "Erreur: diff_to_image a échoué\n");
        goto cleanup;
    }

    printf("Reconstruction OK\n");

    /* Écriture image reconstruite */
    if (!write_image(recon, "reconstructed.pnm")) {
        fprintf(stderr, "Erreur: écriture de reconstructed.pnm\n");
        goto cleanup;
    }

    printf("Image reconstruite écrite dans reconstructed.pnm\n");

cleanup:
    if (diff) {
        free(diff->data);
        free(diff);
    }
    if (recon) {
        free_image(recon);
        free(recon);
    }
    free_image(&img);
    free(image_path);
    return 0;
}
