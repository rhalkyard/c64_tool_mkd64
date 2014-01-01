
#include "../module.h"
#include "../imodule.h"
#include "../debug.h"
#include "../block.h"
#include "../track.h"
#include "../random.h"
#include "../stdintrp.h"

#include <string.h>

static const char *modid = "cbmdos";

typedef struct
{
    IModule mod;
    Image *image;
    Block *bam;
    Block *directory[18];
    int currentDirSector;
} Cbmdos;

static const uint8_t _initialBam[256] = {
    0x12, 0x01, 0x41, 0x00, 0x15, 0xff, 0xff, 0x1f,
    0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f,
    0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f,
    0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f,
    0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f,
    0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f,
    0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f,
    0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f,
    0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f,
    0x13, 0xff, 0xff, 0x07, 0x13, 0xff, 0xff, 0x07,
    0x13, 0xff, 0xff, 0x07, 0x13, 0xff, 0xff, 0x07,
    0x13, 0xff, 0xff, 0x07, 0x13, 0xff, 0xff, 0x07,
    0x13, 0xff, 0xff, 0x07, 0x12, 0xff, 0xff, 0x03,
    0x12, 0xff, 0xff, 0x03, 0x12, 0xff, 0xff, 0x03,
    0x12, 0xff, 0xff, 0x03, 0x12, 0xff, 0xff, 0x03,
    0x12, 0xff, 0xff, 0x03, 0x11, 0xff, 0xff, 0x01,
    0x11, 0xff, 0xff, 0x01, 0x11, 0xff, 0xff, 0x01,
    0x11, 0xff, 0xff, 0x01, 0x11, 0xff, 0xff, 0x01,
    0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
    0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
    0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0x32, 0x41, 0xa0,
    0xa0, 0xa0, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static void
initImage(IModule *this, Image *image)
{
    Cbmdos *dos = (Cbmdos *)this;
    BlockPosition pos = { 18, 0 };
    int i;
    uint8_t *data;

    dos->image = image;
    dos->bam = image_block(image, &pos);
    dos->currentDirSector = 0;

    for (i = 0; i < 18; ++i)
    {
        ++(pos.sector);
        dos->directory[i] = image_block(image, &pos);
        block_reserve(dos->directory[i]);
    }

    data = block_rawData(dos->bam);
    memcpy(data, _initialBam, 256);

    data[0xa2] = random_num(0x30, 0x53);
    if (data[0xa2] > 0x39) data[0xa2] += 7;
    data[0xa3] = random_num(0x30, 0x53);
    if (data[0xa3] > 0x39) data[0xa2] += 7;

    block_allocate(dos->bam);
}

static void
globalOption(IModule *this, char opt, const char *arg)
{
}

static void
fileOption(IModule *this, Diskfile *file, char opt, const char *arg)
{
}

static void
fileWritten(IModule *this, Diskfile *file, const BlockPosition *start)
{
    DBGd2("cbmdos: fileWritten", start->track, start->sector);
}

static void
statusChanged(IModule *this, const BlockPosition *pos)
{
    Cbmdos *dos = (Cbmdos *)this;
    uint8_t *bamEntry;
    int bamByte, bamBit;

    DBGd2("cbmdos: statusChanged", pos->track, pos->sector);

    bamEntry = block_rawData(dos->bam) + 4 * pos->track;
    bamEntry[0] = track_freeSectorsRaw(image_track(dos->image, pos->track));
    bamByte = pos->sector / 8 + 1;
    bamBit = pos->sector % 8;
    if (image_blockStatus(dos->image, pos) & BS_ALLOCATED)
    {
        bamEntry[bamByte] &= ~(1<<bamBit);
    }
    else
    {
        bamEntry[bamByte] |= 1<<bamBit;
    }
}

const char *
id(void)
{
    return modid;
}

IModule *
instance(void)
{
    Cbmdos *this = malloc(sizeof(Cbmdos));
    this->mod.id = &id;
    this->mod.initImage = &initImage;
    this->mod.globalOption = &globalOption;
    this->mod.fileOption = &fileOption;
    this->mod.getTrack = 0;
    this->mod.fileWritten = &fileWritten;
    this->mod.statusChanged = &statusChanged;

    return (IModule *) this;
}

void
delete(IModule *instance)
{
    free(instance);
}

/* vim: et:si:ts=4:sts=4:sw=4
*/
