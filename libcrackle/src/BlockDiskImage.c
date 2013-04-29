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
    BlockDiskImage* pThis = NULL;
    
    __try
    {
        pThis = allocateAndZero(sizeof(*pThis));
        DiskImage_Init(&pThis->super, &BlockDiskImageVTable, blockCount * DISK_IMAGE_BLOCK_SIZE);
    }
    __catch
    {
        DiskImage_Free(&pThis->super);
        __rethrow;
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


__throws void BlockDiskImage_UpdateImageTableFile(BlockDiskImage* pThis, unsigned short newImageTableAddress)
{
    DiskImage_UpdateImageTableFile(&pThis->super, newImageTableAddress);
}


__throws void BlockDiskImage_InsertObjectFile(BlockDiskImage* pThis, DiskImageInsert* pInsert)
{
    DiskImage_InsertObjectFile(&pThis->super, pInsert);
}


static void insertData(void* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    BlockDiskImage_InsertData((BlockDiskImage*)pThis, pData, pInsert);
}


static void insertRW18Data(BlockDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert);
static void validateRW18InsertionProperties(DiskImageInsert* pInsert);
static DiskImageInsert convertRW18SideTrackSectorToBlockAndOffset(DiskImageInsert* pInsert);
static unsigned int startBlockForSide(unsigned short side);
static void insertBlockData(BlockDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert);
static void validateOffsetTypeIsBlock(DiskImageInsert* pInsert);
static void validateImageOffsets(BlockDiskImage* pThis, DiskImageInsert* pInsert);
static unsigned int calculateSourceOffset(DiskImageInsert* pInsert);
__throws void BlockDiskImage_InsertData(BlockDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    if (pInsert->type == DISK_IMAGE_INSERTION_RW18)
        insertRW18Data(pThis, pData, pInsert);
    else
        insertBlockData(pThis, pData, pInsert);
}

static void insertRW18Data(BlockDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    DiskImageInsert insert = *pInsert;
    
    validateRW18InsertionProperties(pInsert);
    insert = convertRW18SideTrackSectorToBlockAndOffset(pInsert);
    insertBlockData(pThis, pData, &insert);
}

static void validateRW18InsertionProperties(DiskImageInsert* pInsert)
{
    if (pInsert->track >= DISK_IMAGE_TRACKS_PER_SIDE)
        __throw(invalidTrackException);
    if (pInsert->sector >= 18)
        __throw(invalidSectorException);
    if (pInsert->intraSectorOffset >= DISK_IMAGE_BYTES_PER_SECTOR)
        __throw(invalidIntraSectorOffsetException);
}

static DiskImageInsert convertRW18SideTrackSectorToBlockAndOffset(DiskImageInsert* pInsert)
{
    DiskImageInsert insert = *pInsert;
    
    unsigned int sectorWithinSide = pInsert->track * DISK_IMAGE_RW18_SECTORS_PER_TRACK + pInsert->sector;
    unsigned int blockWithinSide = sectorWithinSide / DISK_IMAGE_SECTORS_PER_BLOCK;
    insert.type = DISK_IMAGE_INSERTION_BLOCK;
    insert.block = startBlockForSide(pInsert->side) + blockWithinSide;
    insert.intraBlockOffset = (sectorWithinSide % DISK_IMAGE_SECTORS_PER_BLOCK) * DISK_IMAGE_BYTES_PER_SECTOR;
    insert.intraBlockOffset += pInsert->intraSectorOffset;
    
    return insert;
}

static unsigned int startBlockForSide(unsigned short side)
{
    switch (side)
    {
    case 0xa9:
        return 16;
    case 0xad:
        return 16+315+1;
    case 0x79:
        return 16+315+1+315;
    default:
        __throw(invalidSideException);
    }
}

static void insertBlockData(BlockDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    unsigned char* pImage = DiskImage_GetImagePointer(&pThis->super);
    validateOffsetTypeIsBlock(pInsert);
    validateImageOffsets(pThis, pInsert);
    memcpy(pImage + calculateSourceOffset(pInsert), 
           pData + pInsert->sourceOffset, 
           pInsert->length);
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
