#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "image.h"
#include "diff.h"
#include "vlc.h"

void print_help(char *bin) {
    printf("Usage: %s [OPTIONS] -i <input> -o <output>\n", bin);
    printf("Options:\n");
    printf("  -i <file>    Fichier d'entrée (PNM, JPEG, PNG... ou DIF)\n");
    printf("  -o <file>    Fichier de sortie (.dif pour encodage, .pnm pour décodage)\n");
    printf("  -v           Mode verbeux (affiche les détails)\n");
    printf("  -t           Mesurer le temps d'exécution\n");
    printf("  -d           Forcer le mode décodage (DIF -> PNM)\n");
    printf("  -V <prog>    Visualiser l'image de sortie avec <prog> (ex: display, eog)\n");
    printf("  -h           Afficher cette aide\n");
}

int main(int argc, char *argv[]) {
    int opt;
    char *input = NULL, *output = NULL, *viewer = NULL;
    int verbose = 0, measure_time = 0, force_decode = 0;

    while ((opt = getopt(argc, argv, "i:o:V:vthd")) != -1) {
        switch (opt) {
            case 'i': input = strdup(optarg); break;
            case 'o': output = strdup(optarg); break;
            case 'V': viewer = strdup(optarg); break;
            case 'v': verbose = 1; break;
            case 't': measure_time = 1; break;
            case 'd': force_decode = 1; break;
            case 'h': print_help(argv[0]); return 0;
            default: print_help(argv[0]); return 1;
        }
    }

    if (!input || !output) {
        fprintf(stderr, "Erreur: Entrée et sortie obligatoires. Voir -h\n");
        return 1;
    }

    // --- LOGIQUE DE DÉCISION ---
    clock_t start = clock();

    // Détection automatique : si l'entrée finit par .dif ou si -d est présent
    if (force_decode || (strlen(input) > 4 && strcmp(input + strlen(input) - 4, ".dif") == 0)) {
        if (verbose) printf("Mode: Décompression DIF -> PNM\n");
        if (diftopnm(input, output) != 0) return 1;
    } else {
        if (verbose) printf("Mode: Compression Image -> DIF\n");
        if (pnmtodif(input, output) != 0) return 1;
        
        // Calcul du taux de compression (Optionnel)
        // Taux = 100 * (1 - (taille_dif / taille_brute))
    }

    if (measure_time) {
        double diff = (double)(clock() - start) / CLOCKS_PER_SEC;
        printf("Temps d'exécution total: %.3f secondes\n", diff);
    }

    // --- VISUALISEUR ---
    if (viewer && output) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "%s %s &", viewer, output);
        if (verbose) printf("Exécution du visualiseur: %s\n", cmd);
        system(cmd);
    }

    free(input); free(output); if(viewer) free(viewer);
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
