#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "image.h"
#include "diff.h"
#include "normalize.h"
#include "vlc.h"

int main(int argc, char *argv[]) {
    int opt;
    char *image_path = NULL;

    image img = {0};
    diff_image *diff = NULL;
    image *recon = NULL;
    unsigned int *norm = NULL;
    unsigned int *norm_dec = NULL;
    unsigned char *vlc_buf = NULL;


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

    /* diff -> normalisation */
    size_t size = diff->width * diff->height * diff->layer;
    norm = normalize_diff(diff);
    if (!norm) {
        fprintf(stderr, "Erreur: normalize_diff a échoué\n");
        goto cleanup;
    }
    printf("Normalisation OK\n");

    /*Fonctions pour la VLC à mettre ici*/
        /* --- VLC : norm -> flux binaire -> norm_decoded --- */
    size_t vlc_bytes = 0;

    size_t bits_written = vlc_encode(norm, size, &vlc_buf, &vlc_bytes);
    if (!bits_written) {
        fprintf(stderr, "Erreur: vlc_encode a échoué\n");
        goto cleanup;
    }
    printf("VLC encodage OK (%zu bits, %zu octets)\n", bits_written, vlc_bytes);

    norm_dec = malloc(size * sizeof(unsigned int));
    if (!norm_dec) {
        fprintf(stderr, "Erreur: malloc norm_dec\n");
        goto cleanup;
    }

    size_t bits_read = vlc_decode(vlc_buf, vlc_bytes, norm_dec, size);
    if (!bits_read) {
        fprintf(stderr, "Erreur: vlc_decode a échoué\n");
        goto cleanup;
    }
    printf("VLC décodage OK (%zu bits)\n", bits_read);

    /* on peut maintenant utiliser norm_dec pour la dénormalisation */
    denormalize_diff(norm_dec, size, diff);
    printf("Dénormalisation OK\n");

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
    free(norm);
    free(norm_dec);
    free(vlc_buf);
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


// pour tester les fonctions normalize/denormalize (ctrl + / pour commenter/decommenter)
// int main(void) {
//     diff_image diff;
//     int16_t data[] = {
//         0, -1, 1, -2, 2, -5, 5, -10, 10
//     };

//     diff.data = data;
//     diff.width = 3;
//     diff.height = 3;
//     diff.layer = 1;

//     size_t size = 9;

//     unsigned int *norm = normalize_diff(&diff);
//     if (!norm) {
//         fprintf(stderr, "normalize_diff a échoué\n");
//         return 1;
//     }

//     diff_image back;
//     back.data = malloc(size * sizeof(int16_t));

//     denormalize_diff(norm, size, &back);

//     printf("diff  -> norm -> diff\n");
//     for (size_t i = 0; i < size; i++) {
//         printf("%4d -> %4u -> %4d\n",
//                diff.data[i], norm[i], back.data[i]);
//     }

//     free(norm);
//     free(back.data);
//     return 0;
// }

// int main(void) {
//     unsigned int input[] = {0,1,2,3,4,5,10,19,20,30,40,51,52,100,200,255};
//     size_t n = sizeof(input)/sizeof(input[0]);
//     unsigned char* buf = NULL;
//     size_t bytes = 0;

//     vlc_encode(input, n, &buf, &bytes);

//     unsigned int output[n];
//     vlc_decode(buf, bytes, output, n);

//     for (size_t i = 0; i < n; i++)
//         printf("%3u -> %3u\n", input[i], output[i]);

//     free(buf);
//     return 0;
// }
