#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "image.h" 

int visu_existe(const char *visu) {
    char cmd[256];
    char buffer[256];
    FILE *fp;
    snprintf(cmd, sizeof(cmd), "which %s 2>/dev/null", visu); 
    fp = popen(cmd, "r");
    if (fp == NULL) {
        fprintf(stderr, "Erreur lors de l'exécution de popen.\n");
        return 0;
    }
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        pclose(fp);
        return 1;
    }
    pclose(fp);
    return 0;
}

int check_file(const char *filepath, const char *category, const char *format, const char *encoding) {
    char cmd[512];
    char mime_output[256];
    char actual_cat[64] = "";
    char actual_fmt[64] = "";
    char actual_enc[64] = "";
    FILE *fp = NULL;
    if (access(filepath, F_OK) != 0) {
        fprintf(stderr, "Erreur: Le fichier '%s' n'existe pas ou n'est pas accessible.\n", filepath);
        return 0;
    }
    snprintf(cmd, sizeof(cmd), "file -ib \"%s\" 2>&1", filepath);
    fp = popen(cmd, "r");
    if (fp == NULL) {
        fprintf(stderr, "Erreur lors de l'exécution de popen.\n");
        return 0;
    }
    if (fgets(mime_output, sizeof(mime_output), fp) == NULL) {
        pclose(fp);
        fprintf(stderr, "Erreur: 'file -ib' n'a pas retourné d'information.\n");
        return 0;
    }
    pclose(fp);
    mime_output[strcspn(mime_output, "\n")] = 0;
    if (strstr(mime_output, "cannot open") != NULL || strstr(mime_output, "No such file or directory") != NULL) {
        fprintf(stderr, "Erreur de fichier détectée par 'file': %s\n", mime_output);
        return 0;
    }
    if (sscanf(mime_output, "%[^/]/%[^;]; charset=%s", actual_cat, actual_fmt, actual_enc) != 3) {
        if (sscanf(mime_output, "%[^/]/%s", actual_cat, actual_fmt) != 2) {
             fprintf(stderr, "Avertissement: Format MIME inattendu ou incomplet: %s\n", mime_output);
        }
    }
    // Vérification de la CATÉGORIE
    if (category != NULL && category[0] != '0' && category[0] != '\0') {
        if (strcmp(category, actual_cat) != 0) {
            fprintf(stderr, "Échec de vérification: Catégorie attendue '%s', trouvée '%s'.\n", category, actual_cat);
            return 0;
        }
    }
    // Vérification du FORMAT
    if (format != NULL && format[0] != '0' && format[0] != '\0') {
        if (strcmp(format, actual_fmt) != 0) {
             fprintf(stderr, "Échec de vérification: Format attendu '%s', trouvé '%s'.\n", format, actual_fmt);
            return 0;
        }
    }
    // Vérification de l'ENCODAGE
    if (encoding != NULL && encoding[0] != '0' && encoding[0] != '\0') {
        if (strcmp(encoding, actual_enc) != 0) {
            fprintf(stderr, "Échec de vérification: Encodage attendu '%s', trouvé '%s'.\n", encoding, actual_enc);
            return 0;
        }
    }
    return 1;
}

void free_image(image *img) {
    if (img != NULL && img->pixmap != NULL) {
        free(img->pixmap);
        img->pixmap = NULL;
        printf("Mémoire du pixmap libérée.\n");
    }
}

// Fonction pour sauter les espaces blancs et commentaires
static int skip_whitespace_and_comments(FILE *f) {
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '#') {
            // Sauter tout jusqu'à la fin de la ligne
            while ((c = fgetc(f)) != EOF && c != '\n');
        } else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            // C'est un caractère significatif, on le remet
            ungetc(c, f);
            return 1;
        }
    }
    return 0; // EOF atteint
}

// Vérifie si le fichier est déjà au format PNM (sans les restrictions strictes de check_file)
static int is_pnm_format(const char *filepath) {
    char cmd[512];
    char mime_output[256];
    FILE *fp;
    
    if (access(filepath, F_OK) != 0) {
        return 0;
    }
    
    snprintf(cmd, sizeof(cmd), "file -ib \"%s\" 2>&1", filepath);
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return 0;
    }
    
    if (fgets(mime_output, sizeof(mime_output), fp) == NULL) {
        pclose(fp);
        return 0;
    }
    pclose(fp);
    
    // Vérifier si c'est un format PNM (graymap ou greymap ou pixmap)
    if (strstr(mime_output, "x-portable-graymap") != NULL ||
        strstr(mime_output, "x-portable-greymap") != NULL ||
        strstr(mime_output, "x-portable-pixmap") != NULL) {
        return 1;
    }
    
    return 0;
}

// Ouvre le fichier image (directement ou via conversion)
static FILE* open_image_file(const char *path_to_file, bool *is_popen) {
    *is_popen = false;
    
    // Vérifier si le fichier existe
    if (access(path_to_file, F_OK) != 0) {
        fprintf(stderr, "open_image_file: Le fichier '%s' n'existe pas.\n", path_to_file);
        return NULL;
    }
    
    // Vérifier si c'est déjà un fichier PNM avec la commande file
    if (is_pnm_format(path_to_file)) {
        // C'est déjà un fichier PNM, on l'ouvre directement
        FILE *fp = fopen(path_to_file, "rb");
        if (!fp) {
            perror("open_image_file: fopen");
        }
        return fp;
    }
    
    // Ce n'est pas un fichier PNM, conversion nécessaire avec ImageMagick
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "convert \"%s\" pnm:- 2>/dev/null", path_to_file);
    *is_popen = true;
    
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        fprintf(stderr, "open_image_file: Erreur lors de l'exécution de popen/convert.\n");
    }
    return fp;
}

// Lit et valide le magic number (P5 ou P6)
static int read_magic_number(FILE *pnmfile, image *img) {
    if (fscanf(pnmfile, "%2s", img->magic_number) != 1) {
        fprintf(stderr, "read_magic_number: Erreur de lecture du Magic-Number.\n");
        return 0;
    }
    img->magic_number[2] = '\0';
    
    if (strcmp(img->magic_number, "P5") == 0) {
        img->layer = 1; // PGM (Niveaux de gris)
    } else if (strcmp(img->magic_number, "P6") == 0) {
        img->layer = 3; // PPM (Couleur RGB)
    } else {
        fprintf(stderr, "read_magic_number: Format PNM non supporté ('%s'). P5 ou P6 attendu.\n", 
                img->magic_number);
        return 0;
    }
    
    return 1;
}

// Lit les dimensions et la valeur max de l'en-tête PNM
static int read_pnm_header(FILE *pnmfile, image *img) {
    // Lecture de la largeur
    skip_whitespace_and_comments(pnmfile);
    if (fscanf(pnmfile, "%zu", &(img->width)) != 1) {
        fprintf(stderr, "read_pnm_header: Erreur de lecture de la largeur.\n");
        return 0;
    }
    
    // Lecture de la hauteur
    skip_whitespace_and_comments(pnmfile);
    if (fscanf(pnmfile, "%zu", &(img->height)) != 1) {
        fprintf(stderr, "read_pnm_header: Erreur de lecture de la hauteur.\n");
        return 0;
    }
    
    // Lecture de la valeur max
    skip_whitespace_and_comments(pnmfile);
    if (fscanf(pnmfile, "%d", &(img->max_val)) != 1) {
        fprintf(stderr, "read_pnm_header: Erreur de lecture de la valeur max.\n");
        return 0;
    }
    
    // Sauter le dernier caractère whitespace avant les données binaires
    skip_whitespace_and_comments(pnmfile);
    
    return 1;
}

// Alloue et lit les données du pixmap
static int read_pixmap_data(FILE *pnmfile, image *img) {
    // Allocation du bloc mémoire
    img->pixmap_size = img->width * img->height * img->layer;
    img->pixmap = (unsigned char *)malloc(img->pixmap_size);
    if (img->pixmap == NULL) {
        fprintf(stderr, "read_pixmap_data: Erreur d'allocation mémoire pour le pixmap.\n");
        return 0;
    }
    
    // Lecture des données pixels en format binaire
    size_t bytes_read = fread(img->pixmap, 1, img->pixmap_size, pnmfile);
    
    // Vérification du nombre d'octets lus
    if (bytes_read != img->pixmap_size) {
        fprintf(stderr, "read_pixmap_data: Erreur de lecture du pixmap. Attendu %zu octets, lus %zu.\n", 
                img->pixmap_size, bytes_read);
        free_image(img);
        return 0;
    }
    
    return 1;
}

// Ferme le fichier (fclose ou pclose selon le type)
static void close_image_file(FILE *fp, bool is_popen) {
    if (fp != NULL) {
        if (is_popen) {
            pclose(fp);
        } else {
            fclose(fp);
        }
    }
}

// Fonction principale : lit et charge une image dans la structure 'img'
int read_image(image *img, const char *path_to_file) {
    bool is_popen = false;
    
    // 1. Ouverture du fichier (direct ou via conversion)
    FILE *pnmfile = open_image_file(path_to_file, &is_popen);
    if (pnmfile == NULL) {
        return 0;
    }
    
    // 2. Lecture du magic number (P5 ou P6)
    if (!read_magic_number(pnmfile, img)) {
        close_image_file(pnmfile, is_popen);
        return 0;
    }
    
    // 3. Lecture de l'en-tête (dimensions et max_val)
    if (!read_pnm_header(pnmfile, img)) {
        close_image_file(pnmfile, is_popen);
        return 0;
    }
    
    // 4. Lecture des données du pixmap
    if (!read_pixmap_data(pnmfile, img)) {
        close_image_file(pnmfile, is_popen);
        return 0;
    }
    
    // 5. Fermeture du fichier
    close_image_file(pnmfile, is_popen);
    
    printf("read_image: Image chargée avec succès: %zu x %zu (layers: %zu, max_val: %d).\n", 
           img->width, img->height, img->layer, img->max_val);
    
    return 1;
}

int write_image(const image *img, const char *path_to_file) {
    FILE *fp = fopen(path_to_file, "wb"); // 'wb' pour l'écriture binaire
    if (fp == NULL) {
        perror("write_image: Erreur lors de l'ouverture du fichier de sortie");
        return 0;
    }

    // 1. Écriture de l'en-tête (Magic-Number, dimensions, valeur max) - Format texte (ASCII) [cite: 257]
    // Ex: P6\n1080 720\n255\n
    if (fprintf(fp, "%s\n%zu %zu\n%d\n", 
                img->magic_number, 
                img->width, 
                img->height, 
                img->max_val) < 0) 
    {
        fprintf(stderr, "write_image: Erreur d'écriture de l'en-tête.\n");
        fclose(fp);
        return 0;
    }

    // 2. Écriture des données pixels (pixmap) - Format binaire [cite: 257]
    size_t bytes_written = fwrite(img->pixmap, 1, img->pixmap_size, fp);
    
    // 3. Vérification
    if (bytes_written != img->pixmap_size) {
        fprintf(stderr, "write_image: Échec d'écriture du pixmap. Attendu %zu octets, écrits %zu.\n", img->pixmap_size, bytes_written);
        fclose(fp);
        return 0;
    }

    // 4. Fermeture du fichier
    if (fclose(fp) == EOF) {
        perror("write_image: Erreur lors de la fermeture du fichier");
        return 0;
    }

    printf("write_image: Image PNM enregistrée avec succès sous '%s'.\n", path_to_file);
    return 1;
}