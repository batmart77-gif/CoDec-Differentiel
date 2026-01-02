#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <stdio.h>

typedef struct
    { unsigned char* ptr; /* pointeur sur l’octet courant */
    size_t cap; /* capacité en lecture/écriture */
} BitStream;

size_t pushbits(unsigned char src, size_t nbit, BitStream* dst);
size_t pullbits(BitStream* src, unsigned char* dst, size_t nbit);

#endif