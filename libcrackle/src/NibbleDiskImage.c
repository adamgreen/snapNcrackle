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
#include "DiskImageTest.h"
#include "BinaryBuffer.h"
#include "TextFile.h"
#include "ParseCSV.h"
#include "util.h"


struct NibbleDiskImage
{
    unsigned char*       pWrite;
    const unsigned char* pSector;
    unsigned char*       pObject;
    TextFile*            pTextFile;
    ParseCSV*            pParser;
    const char*          pScriptFilename;
    unsigned int         objectLength;
    unsigned int         track;
    unsigned int         sector;
    unsigned int         bytesLeft;
    unsigned int         lineNumber;
    DiskImageObject      object;
    unsigned char        checksum;
    unsigned char        lastByte;
    unsigned char        image[NIBBLE_DISK_IMAGE_SIZE];
    unsigned char        aux[86];
};


#define LOG_ERROR(pTHIS, FORMAT, ...) fprintf(stderr, \
                                       "%s:%d: error: " FORMAT LINE_ENDING, \
                                       pTHIS->pScriptFilename, \
                                       pTHIS->lineNumber, \
                                       __VA_ARGS__)


__throws NibbleDiskImage* NibbleDiskImage_Create(void)
{
    NibbleDiskImage* pThis;
    
    __try
    {
        __throwing_func( pThis = allocateAndZero(sizeof(*pThis)) );
        __throwing_func( pThis->pParser = ParseCSV_Create() );
    }
    __catch
    {
        __rethrow_and_return(NULL);
    }
        
    return pThis;
}


static void freeObject(NibbleDiskImage* pThis);
void NibbleDiskImage_Free(NibbleDiskImage* pThis)
{
    if (!pThis)
        return;
    freeObject(pThis);
    ParseCSV_Free(pThis->pParser);
    free(pThis);
}

static void freeObject(NibbleDiskImage* pThis)
{
    free(pThis->pObject);
    pThis->pObject = NULL;
    pThis->objectLength = 0;
}


static void processScriptFromTextFile(NibbleDiskImage* pThis);
static void processNextScriptLine(NibbleDiskImage* pThis, char* pScriptLine);
static void reportScriptLineException(NibbleDiskImage* pThis, const char** ppFields);
static void closeTextFile(NibbleDiskImage* pThis);
__throws void NibbleDiskImage_ProcessScriptFile(NibbleDiskImage* pThis, const char* pScriptFilename)
{
    __try
        pThis->pTextFile = TextFile_CreateFromFile(pScriptFilename);
    __catch
        __rethrow;
    pThis->pScriptFilename = pScriptFilename;
    
    processScriptFromTextFile(pThis);
}

static void processScriptFromTextFile(NibbleDiskImage* pThis)
{
    char* pNextLine;

    pThis->lineNumber = 1;
    while ((pNextLine = TextFile_GetNextLine(pThis->pTextFile)) != NULL)
    {
        processNextScriptLine(pThis, pNextLine);
        pThis->lineNumber++;
    }
    closeTextFile(pThis);
}

static void processNextScriptLine(NibbleDiskImage* pThis, char* pScriptLine)
{
    const char** ppFields;
    
    ParseCSV_Parse(pThis->pParser, pScriptLine);
    if (ParseCSV_FieldCount(pThis->pParser) != 6)
    {
        LOG_ERROR(pThis, 
                  "%s doesn't contain correct fields: RWTS16|RWTS18,filename,startOffset,length,startTrack,startSector",
                  "Line");
        return;
    }
    ppFields = ParseCSV_FieldPointers(pThis->pParser);
    
    __try
    {
        __throwing_func( NibbleDiskImage_ReadObjectFile(pThis, ppFields[1]) );
        pThis->object.startOffset = strtoul(ppFields[2], NULL, 0);
        pThis->object.length = strtoul(ppFields[3], NULL, 0);
        pThis->object.track = strtoul(ppFields[4], NULL, 0);
        pThis->object.sector = strtoul(ppFields[5], NULL, 0);
        __throwing_func( NibbleDiskImage_InsertObjectFileAsRWTS16(pThis, &pThis->object) );
    }
    __catch
    {
        reportScriptLineException(pThis, ppFields);
        __nothrow;
    }
}

static void reportScriptLineException(NibbleDiskImage* pThis, const char** ppFields)
{
    int exceptionCode = getExceptionCode();
    
    if (exceptionCode == fileException)
        LOG_ERROR(pThis, "Failed to read '%s' object file.", ppFields[1]);
    if (exceptionCode == invalidArgumentException)
        LOG_ERROR(pThis, "%s object insertion attribute on this line.", "Invalid");
}

static void closeTextFile(NibbleDiskImage* pThis)
{
    TextFile_Free(pThis->pTextFile);
    pThis->pTextFile = NULL;
}


__throws void NibbleDiskImage_ProcessScript(NibbleDiskImage* pThis, char* pScriptText)
{
    __try
        pThis->pTextFile = TextFile_CreateFromString(pScriptText);
    __catch
        __rethrow;
    pThis->pScriptFilename = "filename";

    processScriptFromTextFile(pThis);
}


static FILE* openFile(const char* pFilename, const char* pMode);
static size_t readFile(void* pBuffer, size_t bytesToRead, FILE* pFile);
static unsigned int roundUpLengthToPageSize(unsigned int length);
static void* allocateMemory(size_t allocationSize);
__throws void NibbleDiskImage_ReadObjectFile(NibbleDiskImage* pThis, const char* pFilename)
{
    FILE*         pFile;
    SavFileHeader header;
    unsigned int  roundedObjectSize;
    
    __try
    {
        unsigned int paddingByteCount;
    
        freeObject(pThis);
        __throwing_func( pFile = openFile(pFilename, "r") );
        __throwing_func( readFile(&header, sizeof(header), pFile) );
        roundedObjectSize = roundUpLengthToPageSize(header.length);
        __throwing_func( pThis->pObject = allocateMemory(roundedObjectSize) );
        __throwing_func( readFile(pThis->pObject, header.length, pFile) );
        paddingByteCount = roundedObjectSize - header.length;
        memset(pThis->pObject + header.length, 0, paddingByteCount);
        pThis->objectLength = roundedObjectSize;
    }
    __catch
    {
    }
    
    fclose(pFile);    
}

static FILE* openFile(const char* pFilename, const char* pMode)
{
    FILE* pFile = fopen(pFilename, pMode);
    if (!pFile)
        __throw_and_return(fileException, pFile);
    return pFile;
}

static size_t readFile(void* pBuffer, size_t bytesToRead, FILE* pFile)
{
    size_t bytesRead = fread(pBuffer, 1, bytesToRead, pFile);
    if (bytesRead != bytesToRead)
        __throw_and_return(fileException, bytesRead);
    return bytesRead;
}

static unsigned int roundUpLengthToPageSize(unsigned int length)
{
    return (length + (DISK_IMAGE_PAGE_SIZE - 1)) & ~(DISK_IMAGE_PAGE_SIZE - 1);
}

static void* allocateMemory(size_t allocationSize)
{
    void* pAlloc = malloc(allocationSize);
    if (!pAlloc)
        __throw_and_return(outOfMemoryException, NULL);
    return pAlloc;
}


static void validateObjectAttributes(NibbleDiskImage* pThis, DiskImageObject* pObject);
__throws void NibbleDiskImage_InsertObjectFileAsRWTS16(NibbleDiskImage* pThis, DiskImageObject* pObject)
{
    __try
        validateObjectAttributes(pThis, pObject);
    __catch
        __rethrow;
    
    NibbleDiskImage_InsertDataAsRWTS16(pThis, pThis->pObject, pObject);
}

static void validateObjectAttributes(NibbleDiskImage* pThis, DiskImageObject* pObject)
{
    if (pObject->startOffset + pObject->length > pThis->objectLength)
        __throw(invalidArgumentException);
}


static void prepareForFirstSector(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageObject* pObject);
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
__throws void NibbleDiskImage_InsertDataAsRWTS16(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageObject* pObject)
{
    prepareForFirstSector(pThis, pData, pObject);
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

static void prepareForFirstSector(NibbleDiskImage* pThis, const unsigned char* pData, DiskImageObject* pObject)
{
    pThis->track = pObject->track;
    pThis->sector = pObject->sector;
    pThis->bytesLeft = pObject->length;
    pThis->pSector = pData + pObject->startOffset;
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
    unsigned int                 startOffset = NIBBLE_DISK_IMAGE_NIBBLES_PER_TRACK * pThis->track + 
                                               NIBBLE_DISK_IMAGE_RWTS16_NIBBLES_PER_SECTOR * pThis->sector;
    const unsigned char*         pStart;
    int                          encodedSize;
    
    __try
        validateTrackAndSector(pThis);
    __catch
        __rethrow;
        
    pThis->pWrite = pThis->image + startOffset;
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
    if (pThis->sector >= NIBBLE_DISK_IMAGE_RWTS16_SECTORS_PER_TRACK ||
        pThis->track >= NIBBLE_DISK_IMAGE_TRACKS_PER_DISK)
    {
        __throw(invalidArgumentException);
    }
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
    size_t bytesWritten = 0;
    
    FILE* pFile = fopen(pImageFilename, "w");
    if (!pFile)
        __throw(fileException);
        
    bytesWritten = fwrite(pThis->image, 1, sizeof(pThis->image), pFile);
    fclose(pFile);
    if (bytesWritten != sizeof(pThis->image))
        __throw(fileException);
}


const unsigned char* NibbleDiskImage_GetImagePointer(NibbleDiskImage* pThis)
{
    return pThis->image;
}


size_t NibbleDiskImage_GetImageSize(NibbleDiskImage* pThis)
{
    return NIBBLE_DISK_IMAGE_SIZE;
}
