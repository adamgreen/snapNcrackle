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
#include "NibbleDiskImage.h"
#include "DiskImagePriv.h"
#include "DiskImageTest.h"
#include "BinaryBuffer.h"
#include "TextFile.h"
#include "ParseCSV.h"
#include "util.h"


struct NibbleDiskImage
{
    DiskImage            super;
    unsigned char*       pWrite;
    const unsigned char* pSector;
    unsigned int         track;
    unsigned int         sector;
    unsigned int         bytesLeft;
    unsigned char        checksum;
    unsigned char        lastByte;
    unsigned char        aux[86];
};


static void freeObject(void* pThis);
static void insertData(void* pThis, const unsigned char* pData, DiskImageInsert* pInsert);
struct DiskImageVTable NibbleDiskImageVTable = 
{ 
    freeObject,
    insertData 
};


__throws NibbleDiskImage* NibbleDiskImage_Create(void)
{
    NibbleDiskImage* pThis;
    
    __try
    {
        __throwing_func( pThis = allocateAndZero(sizeof(*pThis)) );
        __throwing_func( pThis->super = DiskImage_Init(&NibbleDiskImageVTable, NIBBLE_DISK_IMAGE_SIZE) );
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


__throws void NibbleDiskImage_ProcessScriptFile(NibbleDiskImage* pThis, const char* pScriptFilename)
{
    DiskImage_ProcessScriptFile(&pThis->super, pScriptFilename);
}


__throws void NibbleDiskImage_ProcessScript(NibbleDiskImage* pThis, char* pScriptText)
{
    DiskImage_ProcessScript(&pThis->super, pScriptText);
}


__throws void NibbleDiskImage_ReadObjectFile(NibbleDiskImage* pThis, const char* pFilename)
{
    DiskImage_ReadObjectFile(&pThis->super, pFilename);
}


__throws void NibbleDiskImage_InsertObjectFile(NibbleDiskImage* pThis, DiskImageInsert* pInsert)
{
    DiskImage_InsertObjectFile(&pThis->super, pInsert);
}


static void insertData(void* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    NibbleDiskImage_InsertData((NibbleDiskImage*)pThis, pData, pInsert);
}

static void validateOffsetType(DiskImageInsert* pInsert);
static void prepareForFirstSector(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert);
static void advanceToNextSector(NibbleDiskImage* pThis);
static void writeRWTS16Sector(NibbleDiskImage* pThis);
static void validateTrackAndSector(NibbleDiskImage* pThis);
static void writeSyncBytes(NibbleDiskImage* pThis, size_t syncByteCount);
static void writeAddressField(NibbleDiskImage* pThis, unsigned char volume, unsigned char track, unsigned char sector);
static void writeAddressFieldProlog(NibbleDiskImage* pThis);
static void initChecksum(NibbleDiskImage* pThis);
static void write4and4Data(NibbleDiskImage* pThis, unsigned char byte);
static void updateChecksum(NibbleDiskImage* pThis, unsigned char byte);
static void writeFieldEpilog(NibbleDiskImage* pThis);
static void writeDataField(NibbleDiskImage* pThis, const unsigned char* pData);
static void writeDataFieldProlog(NibbleDiskImage* pThis);
static void write6and2Data(NibbleDiskImage* pThis, const unsigned char* pData);
static void fillAuxBuffer(NibbleDiskImage* pThis, const unsigned char* pData);
static unsigned char encodeAuxByte(NibbleDiskImage* pThis, size_t i, const unsigned char* pData);
static unsigned char lowBitsByte(size_t i, const unsigned char* pData);
static unsigned char lowBitOffset(size_t i);
static unsigned char midBitsByte(size_t i, const unsigned char* pData);
static unsigned char midBitOffset(size_t i);
static unsigned char highBitsByte(size_t i, const unsigned char* pData);
static unsigned char highBitOffset(size_t i);
static void checksumNibbilizeAndWrite(NibbleDiskImage* pThis, const unsigned char* pData);
static void checksumNibbilizeAndWriteAuxBuffer(NibbleDiskImage* pThis);
static unsigned char nibbilizeByte(NibbleDiskImage* pThis, unsigned char byte);
static void checksumNibbilizeAndWriteDataBuffer(NibbleDiskImage* pThis, const unsigned char* pData);
static void nibbilizeAndWriteChecksum(NibbleDiskImage* pThis);
__throws void NibbleDiskImage_InsertData(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    __try
    {
        __throwing_func( validateOffsetType(pInsert) );
        prepareForFirstSector(pThis, pData, pInsert);
    }
    __catch
    {
        __rethrow;
    }
    
    while (pThis->bytesLeft > 0)
    {
        __try
        {
            __throwing_func( writeRWTS16Sector(pThis) );
            advanceToNextSector(pThis);
        }
        __catch
        {
            __rethrow;
        }
    }
}

static void validateOffsetType(DiskImageInsert* pInsert)
{
    if (pInsert->type != DISK_IMAGE_INSERTION_RWTS16)
        __throw(invalidInsertionTypeException);
}

static void prepareForFirstSector(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageInsert* pInsert)
{
    pThis->track = pInsert->track;
    pThis->sector = pInsert->sector;
    pThis->bytesLeft = pInsert->length;
    pThis->pSector = pData + pInsert->sourceOffset;
}

static void advanceToNextSector(NibbleDiskImage* pThis)
{
    pThis->bytesLeft -= DISK_IMAGE_BYTES_PER_SECTOR;
    pThis->pSector += DISK_IMAGE_BYTES_PER_SECTOR;
    pThis->sector++;
    if (pThis->sector >= NIBBLE_DISK_IMAGE_RWTS16_SECTORS_PER_TRACK)
    {
        pThis->sector = 0;
        pThis->track++;
    }
}

static void writeRWTS16Sector(NibbleDiskImage* pThis)
{
    static const unsigned char   volume = 0;
    unsigned int                 sourceOffset = NIBBLE_DISK_IMAGE_NIBBLES_PER_TRACK * pThis->track + 
                                               NIBBLE_DISK_IMAGE_RWTS16_NIBBLES_PER_SECTOR * pThis->sector;
    const unsigned char*         pStart;
    int                          encodedSize;
    
    __try
        validateTrackAndSector(pThis);
    __catch
        __rethrow;
        
    pThis->pWrite = DiskImage_GetImagePointer(&pThis->super) + sourceOffset;
    pStart = pThis->pWrite;
    
    writeSyncBytes(pThis, 21);
    writeAddressField(pThis, volume, pThis->track, pThis->sector);
    writeSyncBytes(pThis, 10);
    writeDataField(pThis, pThis->pSector);
    writeSyncBytes(pThis, 22);
    
    encodedSize = pThis->pWrite - pStart;
    assert ( encodedSize == NIBBLE_DISK_IMAGE_RWTS16_NIBBLES_PER_SECTOR );
}

static void validateTrackAndSector(NibbleDiskImage* pThis)
{
    if (pThis->sector >= NIBBLE_DISK_IMAGE_RWTS16_SECTORS_PER_TRACK)
        __throw(invalidSectorException);
    if (pThis->track >= NIBBLE_DISK_IMAGE_TRACKS_PER_DISK)
        __throw(invalidTrackException);
}

static void writeSyncBytes(NibbleDiskImage* pThis, size_t syncByteCount)
{
    memset(pThis->pWrite, 0xff, syncByteCount);
    pThis->pWrite += syncByteCount;
}

static void writeAddressField(NibbleDiskImage* pThis, unsigned char volume, unsigned char track, unsigned char sector)
{
    writeAddressFieldProlog(pThis);
    
    initChecksum(pThis);
    write4and4Data(pThis, volume);
    write4and4Data(pThis, track);
    write4and4Data(pThis, sector);
    write4and4Data(pThis, pThis->checksum);
    
    writeFieldEpilog(pThis);
}

static void writeAddressFieldProlog(NibbleDiskImage* pThis)
{
    memcpy(pThis->pWrite, "\xD5\xAA\x96", 3);
    pThis->pWrite += 3;
}

static void initChecksum(NibbleDiskImage* pThis)
{
    pThis->checksum = 0;
}

static void write4and4Data(NibbleDiskImage* pThis, unsigned char byte)
{
    char oddBits = byte & 0xAA;
    char evenBits = byte & 0x55;
    char encodedOddByte = 0xAA | (oddBits >> 1);
    char encodedEvenByte = 0xAA | evenBits;
    
    updateChecksum(pThis, byte);
    *pThis->pWrite++ = encodedOddByte;
    *pThis->pWrite++ = encodedEvenByte;
}

static void updateChecksum(NibbleDiskImage* pThis, unsigned char byte)
{
    pThis->checksum ^= byte;
}

static void writeFieldEpilog(NibbleDiskImage* pThis)
{
    memcpy(pThis->pWrite, "\xDE\xAA\xEB", 3);
    pThis->pWrite += 3;
}

static void writeDataField(NibbleDiskImage* pThis, const unsigned char* pData)
{
    writeDataFieldProlog(pThis);
    write6and2Data(pThis, pData);
    writeFieldEpilog(pThis);
}

static void writeDataFieldProlog(NibbleDiskImage* pThis)
{
    memcpy(pThis->pWrite, "\xD5\xAA\xAD", 3);
    pThis->pWrite += 3;
}

static void write6and2Data(NibbleDiskImage* pThis, const unsigned char* pData)
{
    fillAuxBuffer(pThis, pData);
    checksumNibbilizeAndWrite(pThis, pData);
}

static void fillAuxBuffer(NibbleDiskImage* pThis, const unsigned char* pData)
{
    size_t i;
    
    for (i = 0; i < sizeof(pThis->aux) ; i++)
        pThis->aux[i] = encodeAuxByte(pThis, i, pData);
}

static unsigned char encodeAuxByte(NibbleDiskImage* pThis, size_t i, const unsigned char* pData)
{
    unsigned char lowByte = lowBitsByte(i, pData);
    unsigned char midByte = midBitsByte(i, pData);
    unsigned char highByte = highBitsByte(i, pData);
    unsigned char lowBits = ((lowByte & 1) << 1) |
                            ((lowByte & 2) >> 1);
    unsigned char midBits = ((midByte & 1) << 3) |
                            ((midByte & 2) << 1);
    unsigned char highBits = ((highByte & 1) << 5) |
                             ((highByte & 2) << 3);
    
    return highBits | midBits | lowBits;
}

static unsigned char lowBitsByte(size_t i, const unsigned char* pData)
{
    return pData[lowBitOffset(i)];
}

static unsigned char lowBitOffset(size_t i)
{
    return (unsigned char)(0x55 - i);
}

static unsigned char midBitsByte(size_t i, const unsigned char* pData)
{
    return pData[midBitOffset(i)];
}

static unsigned char midBitOffset(size_t i)
{
    return (unsigned char)(0xAB - i);
}

static unsigned char highBitsByte(size_t i, const unsigned char* pData)
{
    return pData[highBitOffset(i)];
}

static unsigned char highBitOffset(size_t i)
{
    return (unsigned char)(0x101 - i);
}

static void checksumNibbilizeAndWrite(NibbleDiskImage* pThis, const unsigned char* pData)
{
    pThis->lastByte = 0;
    initChecksum(pThis);
    
    checksumNibbilizeAndWriteAuxBuffer(pThis);
    checksumNibbilizeAndWriteDataBuffer(pThis, pData);
    nibbilizeAndWriteChecksum(pThis);
}

static void checksumNibbilizeAndWriteAuxBuffer(NibbleDiskImage* pThis)
{
    unsigned char* pCurr = &pThis->aux[sizeof(pThis->aux)-1];
    size_t         i;
    
    for (i = 0 ; i < sizeof(pThis->aux) ; i++)
        *pThis->pWrite++ = nibbilizeByte(pThis, *pCurr--);
    
}

static unsigned char nibbilizeByte(NibbleDiskImage* pThis, unsigned char byte)
{
    unsigned char encodedByte;
    static const unsigned char nibbleArray[64] =
    {
        0x96, 0x97, 0x9a, 0x9b, 0x9d, 0x9e, 0x9f, 0xa6,
        0xa7, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb2, 0xb3,
        0xb4, 0xb5, 0xb6, 0xb7, 0xb9, 0xba, 0xbb, 0xbc,
        0xbd, 0xbe, 0xbf, 0xcb, 0xcd, 0xce, 0xcf, 0xd3,
        0xd6, 0xd7, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde,
        0xdf, 0xe5, 0xe6, 0xe7, 0xe9, 0xea, 0xeb, 0xec,
        0xed, 0xee, 0xef, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
        0xf7, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    };
    
    assert ( byte < 64 );
    encodedByte = byte ^ pThis->lastByte;
    pThis->lastByte = byte;

    return nibbleArray[encodedByte];
}

static void checksumNibbilizeAndWriteDataBuffer(NibbleDiskImage* pThis, const unsigned char* pData)
{
    const unsigned char* pCurr = pData;
    size_t         i;
    
    for (i = 0 ; i < DISK_IMAGE_BYTES_PER_SECTOR ; i++)
        *pThis->pWrite++ = nibbilizeByte(pThis, (*pCurr++) >> 2);
    
}

static void nibbilizeAndWriteChecksum(NibbleDiskImage* pThis)
{
    *pThis->pWrite++ = nibbilizeByte(pThis, 0x00);
}


__throws void NibbleDiskImage_WriteImage(NibbleDiskImage* pThis, const char* pImageFilename)
{
    DiskImage_WriteImage(&pThis->super, pImageFilename);
}


const unsigned char* NibbleDiskImage_GetImagePointer(NibbleDiskImage* pThis)
{
    return DiskImage_GetImagePointer(&pThis->super);
}


size_t NibbleDiskImage_GetImageSize(NibbleDiskImage* pThis)
{
    return DiskImage_GetImageSize(&pThis->super);
}
