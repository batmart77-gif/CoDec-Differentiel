#ifndef IMAGE_H
#define IMAGE_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    size_t width; 
    size_t height;
    size_t layer; 
    unsigned char *pixmap;
    int max_val;
    char magic_number[3];
    size_t pixmap_size;
} image;

int visu_existe(const char *visu);
int check_file(const char *filepath, const char *category, const char *format, const char *encoding);
void free_image(image *img);
int read_image(image *img, const char *path_to_file);
int write_image(const image *img, const char *path_to_file);

#endif