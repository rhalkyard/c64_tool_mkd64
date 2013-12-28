
#include <stdint.h>
#include <stdlib.h>

#include "block.h"
#include "blckstat.h"

struct block
{
    BlockStatus status;
    uint8_t data[BLOCK_RAWSIZE];
};

Block *
block_new(void)
{
    Block *this = calloc(1, sizeof(Block));
    return this;
}

void
block_delete(Block *this)
{
    free(this);
}

BlockStatus
block_status(const Block *this)
{
    return this->status;
}

uint8_t
block_nextTrack(const Block *this)
{
    return this->data[0];
}

uint8_t
block_nextSector(const Block *this)
{
    return this->data[1];
}

void
block_nextPosition(const Block *this, BlockPosition *pos)
{
    pos->track = this->data[0];
    pos->sector = this->data[1];
}

void
block_setNextTrack(Block *this, uint8_t nextTrack)
{
    this->data[0] = nextTrack;
}

void
block_setNextSector(Block *this, uint8_t nextSector)
{
    this->data[1] = nextSector;
}

void
block_setNextPosition(Block *this, const BlockPosition *pos)
{
    this->data[0] = pos->track;
    this->data[1] = pos->sector;
}

int
block_reserve(Block *this)
{
    if (this->status & BS_RESERVED) return 0;
    this->status |= BS_RESERVED;
    return 1;
}

int
block_allocate(Block *this)
{
    if (this->status & BS_ALLOCATED) return 0;
    this->status |= BS_ALLOCATED;
    return 1;
}

uint8_t *
block_data(Block *this)
{
    return &(this->data[2]);
}

uint8_t *
block_rawData(Block *this)
{
    return this->data;
}

/* vim: et:si:ts=8:sts=4:sw=4
*/
