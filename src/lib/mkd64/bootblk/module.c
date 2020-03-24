#include <mkd64/common.h>
#include <mkd64/mkd64.h>
#include <mkd64/imodule.h>
#include <mkd64/util.h>
#include <string.h>

#include "bootalloc.h"

MKD64_MODULE("bootblk")

#define MODVERSION "1.0"

int reserveBootBlocks(IModule *, size_t);
int unReserveBootBlocks(IModule *, size_t);

typedef struct
{
    IModule mod;
    /* add runtime properties here */
    Image * image;
    size_t boot_reserved;
    int reserve_hint;
} Module;

static void
delete(IModule *self)
{
    Module *mod = (Module *)self;

    /* add destructor code here, free all memory allocated in instance() */

    free(mod);
}

static int
checkBootable(IModule * self, DiskFile * file, const BlockPosition * start) {
    /* Verify that `file` looks like a valid C128 boot block */
    if (start->track != 1 || start->sector != 0) {
        fprintf(stderr, "Warning: File '%s' marked as bootloader, but does not start at (1,0)\n", DiskFile_name(file));
        return 0;
    }

    Block * bootBlock = Image_block(((Module *) self)->image, start);
   
    /* boot block must start with 0x43 0x42, 0x4d ('CBM') */ 
    unsigned char magic[] = {0x43, 0x42, 0x4d};
    unsigned char * data = (unsigned char *) Block_rawData(bootBlock);
    if (memcmp(data, magic, 3))
    {
        fprintf(stderr, "Warning: File '%s' marked as bootloader, but it does not have 'CBM' as its\n"
                        "         first three bytes.\n", DiskFile_name(file));
        return 0;
    }

    /* Byte 6 of boot block indicates number of extra blocks that should be
     * loaded. Verify that the size of the file is appropriate for the number of
     * blocks */
    size_t extra_blocks = data[6];
    size_t min_file_size = (extra_blocks * BLOCK_RAWSIZE) + 1;
    size_t max_file_size = (extra_blocks + 1) * BLOCK_RAWSIZE;
    size_t actual_file_size = DiskFile_size(file);
    if (actual_file_size < min_file_size)
    {
        fprintf(stderr, "Warning: Bootloader file '%s' does not contain enough data\n"
                        "         (%zu chained blocks - should be at least %zu bytes).\n",
                        DiskFile_name(file), extra_blocks, min_file_size);

        return 0;
    }
    else if (actual_file_size > max_file_size)
    {
        fprintf(stderr, "Warning: Bootloader file '%s' contains too much data\n"\
                        "         (%zu chained blocks - should be %zu bytes or fewer).\n",
                        DiskFile_name(file), extra_blocks, max_file_size);
        return 0;
    }

    return 1;
}

SOEXPORT void
initImage(IModule * self, Image * image) {
    ((Module *) self)->image = image;
    ((Module *) self)->boot_reserved = 0;
    ((Module *) self)->reserve_hint = 0;
    Image_setAllocator(image, &bootBlockAllocator);
}

SOEXPORT int
option(IModule * self, char opt, const char * arg) {
    int handled = 0;
    size_t n;

    switch (opt) {
        case 'B':
            handled = 1;
            checkArgAndWarn(opt, arg, 0, 1, self->id());
            n = strtol(arg, NULL, 10);

            /* if option is given multiple times, clear previously reserved
            blocks before setting reservation */
            if (n < ((Module *) self)->boot_reserved)
            {
                unReserveBootBlocks(self, ((Module *) self)->boot_reserved);
            }

            if (!reserveBootBlocks(self, n)) {
                fprintf(stderr, "Warning: Unable to reserve %zu blocks for bootloader\n", n);
            } else {
                ((Module *) self)->boot_reserved = n;
            }
        break;
    }
    return handled;
}

SOEXPORT int
fileOption(IModule * self, DiskFile * file, char opt, const char * arg) {
    int handled = 0;

    switch (opt) {
        case 'f':
            /* intercept '-f' option so that if we start a new file, make sure
             * that we reset the allocator mode */
            if (bootBlockState == BBS_ACTIVE) bootBlockState = BBS_INACTIVE;
            handled = 1;
            break;
        case 'b':
            /* mark file as bootloader */
            checkArgAndWarn(opt, arg, 1, 0, self->id());

            /* calculate number of blocks required */
            size_t est_blocks = (DiskFile_size(file) / 256) + ((DiskFile_size(file) % 256 ) ? 1 : 0);
            if (est_blocks > ((Module *) self)->boot_reserved)
            {
                /* not enough blocks have been reserved; try to reserve them */
                if (reserveBootBlocks(self, est_blocks)) {
                    ((Module *) self)->boot_reserved = est_blocks;
                }
            }

            if (est_blocks <= ((Module *) self)->boot_reserved)
            {
                if (est_blocks < ((Module *) self)->boot_reserved)
                {
                    /* more blocks have been reserved than are needed; hint for
                     * smaller reservation size */
                    ((Module *) self)->reserve_hint = est_blocks - ((Module *) self)->boot_reserved;
                }
                /* bootloader must be written raw (written to sequential
                 * blocks, with no chaining headers) */
                DiskFile_setRaw(file, 1);
                fprintf(stderr, "Writing '%s' as bootloader\n", DiskFile_name(file));
                bootBlockState = BBS_ACTIVE;
            }
            else
            {
                fprintf(stderr, "Warning: Unable to write '%s' as bootloader - the required sectors could not be reserved.\n", DiskFile_name(file));
                ((Module *) self)->reserve_hint = est_blocks - ((Module *) self)->boot_reserved;
            }
            handled = 1;
            break;
    }
    return handled;
}

SOEXPORT void
fileWritten(IModule * self, DiskFile * file, const BlockPosition * start) {
    if (bootBlockState == BBS_ACTIVE)
    {
        /* if file had -b flag set, check and warn if the written boot block
        appears incorrect */
        checkBootable(self, file, start);
    }
    bootBlockState = BBS_INACTIVE;
}

SOEXPORT int
requestReservedBlock(IModule * self, const BlockPosition * pos) {
    Image * image = ((Module *) self)->image;
    Block * block = Image_block(image, pos);

    /* only return reserved blocks to ourself */
    if (Block_reservedBy(block) == self) return 1;

    return 0;
}

SOEXPORT void
imageComplete(IModule * self) {
    if (((Module *) self)->reserve_hint) {
        char arg[80];
        sprintf(arg, "%zu", ((Module *) self)->boot_reserved + ((Module *) self)->reserve_hint);
        if (((Module *) self)->reserve_hint > 0)
        {
            Mkd64_suggestOption(MKD64, self, 0, 'B', arg,
                "An insufficient number of sectors were reserved for the bootloader. Reserving space ensures that the bootloader can be written successfully.");
        }
        else if (((Module *) self)->reserve_hint < 0)
        {
            Mkd64_suggestOption(MKD64, self, 0, 'B', arg,
                "More sectors reserved for the bootloader than necessary.");
        }
    }
}

SOEXPORT IModule *
instance(void)
{
    Module *mod = mkd64Alloc(sizeof(Module));
    memset(mod, 0, sizeof(Module));

    mod->mod.id = &id;
    mod->mod.free = &delete;
    mod->mod.initImage = &initImage;
    mod->mod.fileOption = &fileOption;
    mod->mod.option = &option;
    mod->mod.fileWritten = &fileWritten;
    mod->mod.requestReservedBlock = &requestReservedBlock;
    mod->mod.imageComplete = &imageComplete;

    /* add more runtime methods as needed, see imodule.h and modapi.txt */

    return (IModule *) mod;
}

SOLOCAL int
setBootReserved(IModule * self, size_t nblocks, BlockStatus state) {
    /* reserve/release `nblocks` sequential blocks, starting at (1,0)
     * set state = BS_RESERVED to reserve, BS_NONE to release */
    size_t tn = 1;

    Image * image = ((Module *) self)->image;
    
    while (nblocks) {
        Track * track = Image_track(image, tn);
        size_t trackSectors = Track_numSectors(track);
        size_t sn = 0;
        while (nblocks && sn < trackSectors) {
            Block * block = Track_block(track, sn);
            BlockStatus currentState = Block_status(block);
            if (state == BS_RESERVED)
            {
                if ((currentState == BS_RESERVED && Block_reservedBy(block) != self) \
                        || currentState == BS_NONE) {
                    if (!Block_reserve(block, self)) return 0;
                }
                else return 0;
            }
            else if (state == BS_NONE)
            {
                if (Block_reservedBy(block) == self) {
                    Block_unReserve(block);
                }
            }
            sn++;
            nblocks--;
        }
        tn++;
    }
    return 1;
}

SOLOCAL int
reserveBootBlocks(IModule * self, size_t nblocks) {
    return setBootReserved(self, nblocks, BS_RESERVED);
}

SOLOCAL int
unReserveBootBlocks(IModule * self, size_t nblocks) {
    return setBootReserved(self, nblocks, BS_NONE);
}

SOEXPORT const char *
help(void)
{
    return
"bootblk allows a file to be written as a C128 bootloader.\n"\
"\n"\
"* Module options:\n"\
"\n"\
"  -B [N]        reserve N blocks (starting at 1,0) for bootloader.\n";
}

SOEXPORT const char *
helpFile(void)
{
    return
"  -b            write file as bootloader.\n";
}

SOEXPORT const char *
versionInfo(void)
{
    return
"bootblk " MODVERSION "\n";
}

/* vim: et:si:ts=4:sts=4:sw=4
*/
