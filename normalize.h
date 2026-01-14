#ifndef NORMALIZE_H
#define NORMALIZE_H

#include <stddef.h>
#include "diff.h"

unsigned int *normalize_diff(const int16_t *data, size_t size);
void denormalize_diff(const unsigned int *norm, size_t size, int16_t *out_data);

#endif
