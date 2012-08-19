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
#include <assert.h>
#include "BlockDiskImage.h"
#include "DiskImagePriv.h"
#include "DiskImageTest.h"
#include "BinaryBuffer.h"
#include "TextFile.h"
#include "ParseCSV.h"
#include "util.h"


struct BlockDiskImage
{
    DiskImage       super;
};


static void insertData(void* pThis, const unsigned char* pData, DiskImageInsert* pInsert);
struct DiskImageVTable BlockDiskImageVTable = 
{ 
    insertData 
};


__throws BlockDiskImage* BlockDiskImage_Create(unsigned int blockCount)
{
    BlockDiskImage* pThis;
    
    __try
    {
        __throwing_func( pThis = allocateAndZero(sizeof(*pThis)) );
        __throwing_func( pThis->super = DiskImage_Init(&BlockDiskImageVTable, blockCount * DISK_IMAGE_BLOCK_SIZE) );
    }
    __catch
    {
        BlockDiskImage_Free(pThis);
        __rethrow_and_return(NULL);
    }
        
    return pThis;
}


void BlockDiskImage_Free(BlockDiskImage* pThis)
{
    if (!pThis)
        return;
    DiskImage_Free(&pThis->super);
    free(pThis);
}


__throws void BlockDiskImage_ProcessScriptFile(BlockDiskImage* pThis, const char* pScriptFilename)
{
    DiskImage_ProcessScriptFile(&pThis->super, pScriptFilename);
}


__throws void BlockDiskImage_ProcessScript(BlockDiskImage* pThis, char* pScriptText)
{
    DiskImage_ProcessScript(&pThis->super, pScriptText);
}


__throws void BlockDiskImage_ReadObjectFile(BlockDiskImage* pThis, const char* pFilename)
{
    DiskImage_ReadObjectFile(&pThis->super, pFilename);
}


__throws void BlockDiskImage_InsertObjectFile(BlockDiskImage* pThis, DiskImageInsert* pInsert)
{
    DiskImage_InsertObjectFile(&pThis->super, pInsert);
}


static void insertData(void* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    BlockDiskImage_InsertData((BlockDiskImage*)pThis, pData, pInsert);
}


static void validateOffsetTypeIsBlock(DiskImageInsert* pInsert);
static void validateImageOffsets(BlockDiskImage* pThis, DiskImageInsert* pInsert);
__throws void BlockDiskImage_InsertData(BlockDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    unsigned char* pImage = DiskImage_GetImagePointer(&pThis->super);
    __try
    {
        __throwing_func( validateOffsetTypeIsBlock(pInsert) );
        __throwing_func( validateImageOffsets(pThis, pInsert) );
        memcpy(pImage + pInsert->block * DISK_IMAGE_BLOCK_SIZE, 
               pData + pInsert->sourceOffset, 
               pInsert->length);
    }
    __catch
    {
        __rethrow;
    }
}

static void validateOffsetTypeIsBlock(DiskImageInsert* pInsert)
{
    if (pInsert->type != DISK_IMAGE_INSERTION_BLOCK)
        __throw(invalidInsertionTypeException);
}

static void validateImageOffsets(BlockDiskImage* pThis, DiskImageInsert* pInsert)
{
    unsigned int sourceOffset = pInsert->block * DISK_IMAGE_BLOCK_SIZE;
    unsigned int endOffset = sourceOffset + pInsert->length;
    unsigned int imageSize = DiskImage_GetImageSize(&pThis->super);
    
    if (sourceOffset >= imageSize || endOffset > imageSize)
        __throw(blockExceedsImageBoundsException);
}


__throws void BlockDiskImage_WriteImage(BlockDiskImage* pThis, const char* pImageFilename)
{
    DiskImage_WriteImage(&pThis->super, pImageFilename);
}


const unsigned char* BlockDiskImage_GetImagePointer(BlockDiskImage* pThis)
{
    return DiskImage_GetImagePointer(&pThis->super);
}


size_t BlockDiskImage_GetImageSize(BlockDiskImage* pThis)
{
    return DiskImage_GetImageSize(&pThis->super);
}
