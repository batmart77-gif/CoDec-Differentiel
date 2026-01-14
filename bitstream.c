#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <limits.h>
#include "bitstream.h"

size_t pushbits(unsigned int src, size_t nbit, BitStream* dst) {
    size_t bits_written = 0;
    while (nbit > 0) {
        size_t to_write = (nbit <= dst->cap) ? nbit : dst->cap;
        // On décale src (qui est un int maintenant)
        unsigned char aligned_bits = (unsigned char)((src >> (nbit - to_write)) & ((1 << to_write) - 1));

        *dst->ptr |= aligned_bits << (dst->cap - to_write);
        dst->cap -= to_write;
        nbit -= to_write;
        bits_written += to_write;
        
        if (dst->cap == 0) {
            dst->ptr++;
            *dst->ptr = 0; 
            dst->cap = CHAR_BIT;
        }
    }
    return bits_written;
}

size_t pullbits(BitStream* src, unsigned int* dst, size_t nbit) {
    size_t bits_read = 0;
    *dst = 0; // dst est un pointeur vers unsigned int
    while (nbit > 0) {
        size_t to_read = (nbit <= src->cap) ? nbit : src->cap;
        unsigned char extracted_bits = (*src->ptr >> (src->cap - to_read)) & ((1 << to_read) - 1);
        *dst = (*dst << to_read) | extracted_bits;
        src->cap -= to_read;
        nbit -= to_read;
        bits_read += to_read;
        if (src->cap == 0) {
            src->ptr++;
            src->cap = CHAR_BIT;
        }
    }
    return bits_read;
}
