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


#define DISK_IMAGE_BYTES_PER_SECTOR          256
#define DISK_IMAGE_RWTS16_NIBBLES_PER_SECTOR 416
#define DISK_IMAGE_RWTS16_SECTORS_PER_TRACK  16
#define DISK_IMAGE_TRACKS_PER_DISK           35
#define DISK_IMAGE_NIBBLES_PER_TRACK         (DISK_IMAGE_RWTS16_NIBBLES_PER_SECTOR * DISK_IMAGE_RWTS16_SECTORS_PER_TRACK)
#define DISK_IMAGE_SIZE                      (DISK_IMAGE_NIBBLES_PER_TRACK * DISK_IMAGE_TRACKS_PER_DISK)
#define DISK_IMAGE_PAGE_SIZE                 256


typedef enum DiskImageOffsetType
{
    DISK_IMAGE_OFFSET_TRACK_SECTOR,
    DISK_IMAGE_OFFSET_BLOCK
} DiskImageOffsetType;

typedef struct DiskImageObject
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
} DiskImageObject;

typedef struct DiskImage DiskImage;


__throws DiskImage* DiskImage_Create(void);
         void       DiskImage_Free(DiskImage* pThis);

__throws void       DiskImage_ProcessScriptFile(DiskImage* pThis, const char* pScriptFilename);
__throws void       DiskImage_ProcessScript(DiskImage* pThis, char* pScriptText);
__throws void       DiskImage_ReadObjectFile(DiskImage* pThis, const char* pFilename);
__throws void       DiskImage_InsertObjectFileAsRWTS16(DiskImage* pThis, DiskImageObject* pObject);
__throws void       DiskImage_InsertDataAsRWTS16(DiskImage*           pThis, 
                                                 const unsigned char* pData, 
                                                 DiskImageObject*     pObject);
                                                   
__throws void       DiskImage_WriteImage(DiskImage* pThis, const char* pImageFilename);
         
         const unsigned char* DiskImage_GetImagePointer(DiskImage* pThis);
         size_t               DiskImage_GetImageSize(DiskImage* pThis);
         
#endif /* _DISK_IMAGE_H_ */
