/*  Copyright (C) 2012  Adam Green (https://github.com/adamgreen)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
#ifndef _DISK_IMAGE_H_
#define _DISK_IMAGE_H_

#include "try_catch.h"


#define DISK_IMAGE_BYTES_PER_SECTOR 256
#define DISK_IMAGE_PAGE_SIZE        256
#define DISK_IMAGE_BLOCK_SIZE       (2 * DISK_IMAGE_BYTES_PER_SECTOR)


typedef enum DiskImageOffsetType
{
    DISK_IMAGE_OFFSET_TRACK_SECTOR,
    DISK_IMAGE_OFFSET_BLOCK
} DiskImageOffsetType;


typedef struct DiskImageInsert
{
    DiskImageOffsetType offsetType;
    unsigned int        startOffset;
    unsigned int        length;
    union
    {
        struct
        {
            unsigned int track;
            unsigned int sector;
        };
        unsigned int block;
    };
} DiskImageInsert;

#endif /* _DISK_IMAGE_H_ */
