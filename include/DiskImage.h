/*  Copyright (C) 2013  Adam Green (https://github.com/adamgreen)

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


#define DISK_IMAGE_BYTES_PER_SECTOR       256
#define DISK_IMAGE_PAGE_SIZE              256
#define DISK_IMAGE_SECTORS_PER_BLOCK      2
#define DISK_IMAGE_BLOCK_SIZE             (DISK_IMAGE_SECTORS_PER_BLOCK * DISK_IMAGE_BYTES_PER_SECTOR)
#define DISK_IMAGE_TRACKS_PER_SIDE        35
#define DISK_IMAGE_RW18_SIDE_0            0xa9
#define DISK_IMAGE_RW18_SIDE_1            0xad
#define DISK_IMAGE_RW18_SIDE_2            0x79
#define DISK_IMAGE_RW18_SECTORS_PER_TRACK 18
#define DISK_IMAGE_RW18_BYTES_PER_TRACK   (DISK_IMAGE_RW18_SECTORS_PER_TRACK * DISK_IMAGE_BYTES_PER_SECTOR)


typedef struct DiskImage DiskImage;


typedef enum DiskImageInsertionType
{
    DISK_IMAGE_INSERTION_RWTS16,
    DISK_IMAGE_INSERTION_RW18,
    DISK_IMAGE_INSERTION_BLOCK
} DiskImageInsertionType;


typedef struct DiskImageInsert
{
    DiskImageInsertionType type;
    unsigned int           sourceOffset;
    unsigned int           length;
    union
    {
        struct
        {
            unsigned int   side;
            unsigned int   track;
            unsigned int   sector;
            unsigned int   intraSectorOffset;
        };
        struct
        {
            unsigned int   block;
            unsigned int   intraBlockOffset;
        };
    };
} DiskImageInsert;


         void      DiskImage_Free(DiskImage* pThis);

__throws void      DiskImage_ProcessScriptFile(DiskImage* pThis, const char*  pScriptFilename);
__throws void      DiskImage_ProcessScript(DiskImage* pThis, char* pScriptText);

__throws void      DiskImage_ReadObjectFile(DiskImage* pThis, const char* pFilename);
__throws void      DiskImage_UpdateImageTableFile(DiskImage* pThis, unsigned short newImageTableAddress);
__throws void      DiskImage_InsertObjectFile(DiskImage* pThis, DiskImageInsert* pInsert);

__throws void      DiskImage_WriteImage(DiskImage* pThis, const char* pImageFilename);

         unsigned char* DiskImage_GetImagePointer(DiskImage* pThis);
         size_t         DiskImage_GetImageSize(DiskImage* pThis);

#endif /* _DISK_IMAGE_H_ */
