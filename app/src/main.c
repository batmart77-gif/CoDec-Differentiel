#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <codec.h>


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
    if (force_decode || (strlen(input) > 4 && strcmp(input + strlen(input) - 4, ".dif") == 0)) {
        if (verbose) printf("Mode: Décompression DIF -> PNM\n");
        if (diftopnm(input, output) != 0) return 1;
    } else {
        if (verbose) printf("Mode: Compression Image -> DIF\n");
        if (pnmtodif(input, output) != 0) return 1;
    }

    if (measure_time) {
        double diff = (double)(clock() - start) / CLOCKS_PER_SEC;
        printf("Temps d'exécution total: %.3f secondes\n", diff);
    }

    // --- VISUALISEUR ---
    if (viewer && output) {
        if (visu_existe(viewer)) {
            char cmd[512];
            // On ajoute des guillemets autour de l'output au cas où le chemin contient des espaces
            snprintf(cmd, sizeof(cmd), "%s \"%s\" &", viewer, output);
            if (verbose) printf("Exécution du visualiseur: %s\n", cmd);
            system(cmd);
        } else {
            fprintf(stderr, "Erreur: Le visualiseur '%s' est introuvable sur ce système.\n", viewer);
        }
    }

    free(input); free(output); if(viewer) free(viewer);
    return 0;
}