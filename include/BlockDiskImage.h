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
#ifndef _BLOCK_DISK_IMAGE_H_
#define _BLOCK_DISK_IMAGE_H_

#include "try_catch.h"
#include "DiskImage.h"


#define BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT    1600
#define BLOCK_DISK_IMAGE_3_5_DISK_SIZE      (DISK_IMAGE_BLOCK_SIZE * BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT)


typedef struct BlockDiskImage BlockDiskImage;


__throws BlockDiskImage* BlockDiskImage_Create(unsigned int blockCount);
         void            BlockDiskImage_Free(BlockDiskImage* pThis);

__throws void            BlockDiskImage_ProcessScriptFile(BlockDiskImage* pThis, const char* pScriptFilename);
__throws void            BlockDiskImage_ProcessScript(BlockDiskImage* pThis, char* pScriptText);
__throws void            BlockDiskImage_ReadObjectFile(BlockDiskImage* pThis, const char* pFilename);
__throws void            BlockDiskImage_InsertObjectFile(BlockDiskImage* pThis, DiskImageInsert* pInsert);
__throws void            BlockDiskImage_InsertData(BlockDiskImage*      pThis, 
                                                   const unsigned char* pData, 
                                                   DiskImageInsert*     pInsert);
                                                   
__throws void            BlockDiskImage_WriteImage(BlockDiskImage* pThis, const char* pImageFilename);
         
         const unsigned char* BlockDiskImage_GetImagePointer(BlockDiskImage* pThis);
         size_t               BlockDiskImage_GetImageSize(BlockDiskImage* pThis);
         
#endif /* _BLOCK_DISK_IMAGE_H_ */
