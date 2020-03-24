#include <mkd64/common.h>
#include <mkd64/iblalloc.h>
#include <mkd64/image.h>
#include <mkd64/track.h>
#include <mkd64/block.h>

#include "bootalloc.h"

static Image *_img;
static IBlockAllocator * baseAllocator;

static void setImage(IBlockAllocator *self, Image *image);
static void setInterleave(IBlockAllocator *self, int interleave);
static void setConsiderReserved(IBlockAllocator *self, int resv);
static Block *allocFirstBlock(IBlockAllocator *self);
static Block *allocNextBlock(IBlockAllocator *self, const BlockPosition *pos);

BootBlockState bootBlockState = BBS_INACTIVE;

IBlockAllocator bootBlockAllocator = {
    &setImage,
    &setInterleave,
    &setConsiderReserved,
    &allocFirstBlock,
    &allocNextBlock
};

static void setInterleave(IBlockAllocator *self, int interleave) {
    (void) self; /* unused */

    baseAllocator->setInterleave(baseAllocator, interleave);
}

static void setConsiderReserved(IBlockAllocator *self, int resv) {
    (void) self; /* unused */

    baseAllocator->setConsiderReserved(baseAllocator, resv);
}

static void
setImage(IBlockAllocator *self, Image *image)
{
    (void) self; /* unused */

    baseAllocator = Image_allocator(image);

    bootBlockState = BBS_INACTIVE;
    _img = image;
}

static Block *
allocFirstBlock(IBlockAllocator *self)
{
    (void) self; /* unused */

    if (bootBlockState == BBS_INACTIVE) {
        return baseAllocator->allocFirstBlock(baseAllocator);
    } else {
        /* Writing first boot block; force allocation at (1,0) */
        const BlockPosition pos = {1, 0};
        return Image_allocateAt(_img, &pos);
    }
}

static Block *
allocNextBlock(IBlockAllocator *self, const BlockPosition *pos)
{
    if (bootBlockState == BBS_INACTIVE) {
        return baseAllocator->allocNextBlock(baseAllocator, pos);
    } else {
        /* Writing chained boot block; force allocation at next sequential block */
        Track *t;
        BlockPosition newPos;

        (void) self; /* unused */

        newPos.track = pos->track;
        newPos.sector = pos->sector + 1;

        t = Image_track(_img, newPos.track);
        if (newPos.sector >= Track_numSectors(t)) {
            /* reached end of track; advance to sector 0 of next track */
            newPos.sector = 0;
            newPos.track += 1;
        }

        return Image_allocateAt(_img, &newPos);
    }
}

/* vim: et:si:ts=8:sts=4:sw=4
*/
