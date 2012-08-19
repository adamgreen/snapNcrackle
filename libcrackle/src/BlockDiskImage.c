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


static void freeObject(void* pThis);
static void insertData(void* pThis, const unsigned char* pData, DiskImageInsert* pInsert);
struct DiskImageVTable BlockDiskImageVTable = 
{ 
    freeObject,
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
        DiskImage_Free(&pThis->super);
        __rethrow_and_return(NULL);
    }
        
    return pThis;
}


static void freeObject(void* pThis)
{
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
static unsigned int calculateSourceOffset(DiskImageInsert* pInsert);
__throws void BlockDiskImage_InsertData(BlockDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    unsigned char* pImage = DiskImage_GetImagePointer(&pThis->super);
    __try
    {
        __throwing_func( validateOffsetTypeIsBlock(pInsert) );
        __throwing_func( validateImageOffsets(pThis, pInsert) );
        memcpy(pImage + calculateSourceOffset(pInsert), 
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
    unsigned int sourceOffset = calculateSourceOffset(pInsert);
    unsigned int endOffset = sourceOffset + pInsert->length;
    unsigned int imageSize = DiskImage_GetImageSize(&pThis->super);
    
    if (pInsert->intraBlockOffset >= DISK_IMAGE_BLOCK_SIZE)
        __throw(invalidIntraBlockOffsetException);
    if (sourceOffset >= imageSize || endOffset > imageSize)
        __throw(blockExceedsImageBoundsException);
}

static unsigned int calculateSourceOffset(DiskImageInsert* pInsert)
{
    return pInsert->block * DISK_IMAGE_BLOCK_SIZE + pInsert->intraBlockOffset;
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
