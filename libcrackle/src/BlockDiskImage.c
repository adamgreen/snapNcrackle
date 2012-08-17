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
#include "DiskImageTest.h"
#include "BinaryBuffer.h"
#include "TextFile.h"
#include "ParseCSV.h"
#include "util.h"


struct BlockDiskImage
{
    TextFile*            pTextFile;
    ParseCSV*            pParser;
    const char*          pScriptFilename;
    unsigned char*       pObject;
    unsigned char*       pImage;
    unsigned int         imageSize;
    unsigned int         objectLength;
    unsigned int         lineNumber;
    DiskImageObject      object;
};


#define LOG_ERROR(pTHIS, FORMAT, ...) fprintf(stderr, \
                                       "%s:%d: error: " FORMAT LINE_ENDING, \
                                       pTHIS->pScriptFilename, \
                                       pTHIS->lineNumber, \
                                       __VA_ARGS__)


__throws BlockDiskImage* BlockDiskImage_Create(unsigned int blockCount)
{
    BlockDiskImage* pThis;
    
    __try
    {
        __throwing_func( pThis = allocateAndZero(sizeof(*pThis)) );
        pThis->imageSize = blockCount * BLOCK_DISK_IMAGE_BLOCK_SIZE;
        __throwing_func( pThis->pImage = allocateAndZero(pThis->imageSize) );
        __throwing_func( pThis->pParser = ParseCSV_Create() );
    }
    __catch
    {
        BlockDiskImage_Free(pThis);
        __rethrow_and_return(NULL);
    }
        
    return pThis;
}


static void freeObject(BlockDiskImage* pThis);
void BlockDiskImage_Free(BlockDiskImage* pThis)
{
    if (!pThis)
        return;
    freeObject(pThis);
    ParseCSV_Free(pThis->pParser);
    free(pThis->pImage);
    free(pThis);
}

static void freeObject(BlockDiskImage* pThis)
{
    free(pThis->pObject);
    pThis->pObject = NULL;
    pThis->objectLength = 0;
}


static void processScriptFromTextFile(BlockDiskImage* pThis);
static void processNextScriptLine(BlockDiskImage* pThis, char* pScriptLine);
static void reportScriptLineException(BlockDiskImage* pThis, const char** ppFields);
static void closeTextFile(BlockDiskImage* pThis);
__throws void BlockDiskImage_ProcessScriptFile(BlockDiskImage* pThis, const char* pScriptFilename)
{
    __try
        pThis->pTextFile = TextFile_CreateFromFile(pScriptFilename);
    __catch
        __rethrow;
    pThis->pScriptFilename = pScriptFilename;
    
    processScriptFromTextFile(pThis);
}

static void processScriptFromTextFile(BlockDiskImage* pThis)
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

static void processNextScriptLine(BlockDiskImage* pThis, char* pScriptLine)
{
    const char** ppFields;
    
    ParseCSV_Parse(pThis->pParser, pScriptLine);
    if (ParseCSV_FieldCount(pThis->pParser) != 5)
    {
        LOG_ERROR(pThis, 
                  "%s doesn't contain correct fields: BLOCK,objectFilename,objectStartOffset,objectLength,startBlock",
                  "Line");
        return;
    }
    ppFields = ParseCSV_FieldPointers(pThis->pParser);
    
    __try
    {
        __throwing_func( BlockDiskImage_ReadObjectFile(pThis, ppFields[1]) );
        pThis->object.startOffset = strtoul(ppFields[2], NULL, 0);
        pThis->object.length = strtoul(ppFields[3], NULL, 0);
        pThis->object.offsetType = DISK_IMAGE_OFFSET_BLOCK;
        pThis->object.block = strtoul(ppFields[4], NULL, 0);
        __throwing_func( BlockDiskImage_InsertObjectFile(pThis, &pThis->object) );
    }
    __catch
    {
        reportScriptLineException(pThis, ppFields);
        __nothrow;
    }
}

static void reportScriptLineException(BlockDiskImage* pThis, const char** ppFields)
{
    int exceptionCode = getExceptionCode();
    
    if (exceptionCode == fileException)
        LOG_ERROR(pThis, "Failed to read '%s' object file.", ppFields[1]);
    if (exceptionCode == invalidArgumentException)
        LOG_ERROR(pThis, "%s object insertion attribute on this line.", "Invalid");
}

static void closeTextFile(BlockDiskImage* pThis)
{
    TextFile_Free(pThis->pTextFile);
    pThis->pTextFile = NULL;
}


__throws void BlockDiskImage_ProcessScript(BlockDiskImage* pThis, char* pScriptText)
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
static unsigned int roundUpLengthToBlockSize(unsigned int length);
static void* allocateMemory(size_t allocationSize);
__throws void BlockDiskImage_ReadObjectFile(BlockDiskImage* pThis, const char* pFilename)
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
        roundedObjectSize = roundUpLengthToBlockSize(header.length);
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

static unsigned int roundUpLengthToBlockSize(unsigned int length)
{
    return (length + (BLOCK_DISK_IMAGE_BLOCK_SIZE - 1)) & ~(BLOCK_DISK_IMAGE_BLOCK_SIZE - 1);
}

static void* allocateMemory(size_t allocationSize)
{
    void* pAlloc = malloc(allocationSize);
    if (!pAlloc)
        __throw_and_return(outOfMemoryException, NULL);
    return pAlloc;
}


static void validateObjectAttributes(BlockDiskImage* pThis, DiskImageObject* pObject);
__throws void BlockDiskImage_InsertObjectFile(BlockDiskImage* pThis, DiskImageObject* pObject)
{
    __try
        validateObjectAttributes(pThis, pObject);
    __catch
        __rethrow;
    
    BlockDiskImage_InsertData(pThis, pThis->pObject, pObject);
}

static void validateObjectAttributes(BlockDiskImage* pThis, DiskImageObject* pObject)
{
    if (pObject->startOffset + pObject->length > pThis->objectLength)
        __throw(invalidArgumentException);
}


static void validateOffsetTypeIsBlock(DiskImageObject* pObject);
static void validateImageOffsets(BlockDiskImage* pThis, DiskImageObject* pObject);
__throws void BlockDiskImage_InsertData(BlockDiskImage* pThis, const unsigned char* pData, DiskImageObject* pObject)
{
    __try
    {
        __throwing_func( validateOffsetTypeIsBlock(pObject) );
        __throwing_func( validateImageOffsets(pThis, pObject) );
        memcpy(pThis->pImage + pObject->block * BLOCK_DISK_IMAGE_BLOCK_SIZE, 
               pData + pObject->startOffset, 
               pObject->length);
    }
    __catch
    {
        __rethrow;
    }
}

static void validateOffsetTypeIsBlock(DiskImageObject* pObject)
{
    if (pObject->offsetType != DISK_IMAGE_OFFSET_BLOCK)
        __throw(invalidArgumentException);
}

static void validateImageOffsets(BlockDiskImage* pThis, DiskImageObject* pObject)
{
    unsigned int startOffset = pObject->block * BLOCK_DISK_IMAGE_BLOCK_SIZE;
    unsigned int endOffset = startOffset + pObject->length;
    
    if (startOffset >= pThis->imageSize || endOffset > pThis->imageSize)
        __throw(invalidArgumentException);
}


__throws void BlockDiskImage_WriteImage(BlockDiskImage* pThis, const char* pImageFilename)
{
    size_t bytesWritten = 0;
    
    FILE* pFile = fopen(pImageFilename, "w");
    if (!pFile)
        __throw(fileException);
        
    bytesWritten = fwrite(pThis->pImage, 1, pThis->imageSize, pFile);
    fclose(pFile);
    if (bytesWritten != pThis->imageSize)
        __throw(fileException);
}


const unsigned char* BlockDiskImage_GetImagePointer(BlockDiskImage* pThis)
{
    return pThis->pImage;
}


size_t BlockDiskImage_GetImageSize(BlockDiskImage* pThis)
{
    return pThis->imageSize;
}
