#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "image.h"
#include "diff.h"
#include "normalize.h"
#include "vlc.h"

int main(int argc, char *argv[])
{
    int opt;
    char *input = NULL;
    char *output = NULL;

    while ((opt = getopt(argc, argv, "i:o:")) != -1) {
        switch (opt) {
        case 'i': input = strdup(optarg); break;
        case 'o': output = strdup(optarg); break;
        default:
            fprintf(stderr, "Usage: %s -i image.pnm -o image.dif\n", argv[0]);
            return 1;
        }
    }

    if (!input || !output) {
        fprintf(stderr, "Erreur: -i et -o obligatoires\n");
        return 1;
    }

    if (pnmtodif(input, output) != 0) {
        fprintf(stderr, "pnmtodif a échoué\n");
        return 1;
    }

    printf("Encodage DIF OK : %s → %s\n", input, output);

    free(input);
    free(output);
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
