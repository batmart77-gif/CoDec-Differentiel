#ifndef VLC_H
#define VLC_H

#include <stddef.h>

int vlc_encode(unsigned int *input, size_t n, unsigned char **out_buf, size_t *out_bytes, unsigned char *qbits);
int vlc_decode(const unsigned char *in_buf, size_t in_bytes, unsigned int *out, size_t n, unsigned char *qbits);

#endif