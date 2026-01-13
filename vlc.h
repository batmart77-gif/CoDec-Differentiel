#ifndef VLC_H
#define VLC_H

#include <stddef.h>

size_t vlc_encode(const unsigned int *in, size_t n, unsigned char **out_buf, size_t *out_bytes);
size_t vlc_decode(const unsigned char *in_buf, size_t in_bytes, unsigned int *out, size_t n);

#endif