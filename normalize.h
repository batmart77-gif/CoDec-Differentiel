#ifndef NORMALIZE_H
#define NORMALIZE_H

#include <stddef.h>
#include "diff.h"

unsigned int *normalize_diff(const diff_image *diff);
void denormalize_diff(const unsigned int *norm, size_t size, diff_image *diff);

#endif
