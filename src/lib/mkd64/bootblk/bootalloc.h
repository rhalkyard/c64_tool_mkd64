#ifndef BOOTALLOC_H
#define BOOTALLOC_H

#include <mkd64/iblalloc.h>

typedef enum {
    BBS_INACTIVE,
    BBS_ACTIVE
} BootBlockState;

extern IBlockAllocator bootBlockAllocator;
extern BootBlockState bootBlockState;

#endif

/* vim: et:si:ts=8:sts=4:sw=4
*/
