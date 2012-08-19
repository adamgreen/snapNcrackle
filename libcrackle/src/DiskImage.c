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
#include <string.h>
#include "DiskImagePriv.h"
#include "DiskImageTest.h"
#include "BinaryBuffer.h"
#include "util.h"


static void DiskImageScriptEngine_Init(DiskImageScriptEngine* pThis);
__throws DiskImage DiskImage_Init(DiskImageVTable* pVTable, unsigned int imageSize)
{
    DiskImage diskImage;
    
    __try
    {
        memset(&diskImage, 0, sizeof(diskImage));
        diskImage.pVTable = pVTable;
        __throwing_func( ByteBuffer_Allocate(&diskImage.image, imageSize) );
        __throwing_func( DiskImageScriptEngine_Init(&diskImage.script) );
    }
    __catch
    {
        __rethrow_and_return(diskImage);
    }
    
    return diskImage;
}

static void DiskImageScriptEngine_Init(DiskImageScriptEngine* pThis)
{
    __try
        pThis->pParser = ParseCSV_Create();
    __catch
        __rethrow;
}


static void DiskImageScriptEngine_Free(DiskImageScriptEngine* pThis);
void DiskImage_Free(DiskImage* pThis)
{
    if (!pThis)
        return;
    
    pThis->pVTable->freeObject(pThis);
    ByteBuffer_Free(&pThis->object);
    ByteBuffer_Free(&pThis->image);
    DiskImageScriptEngine_Free(&pThis->script);
    free(pThis);
}

static void DiskImageScriptEngine_Free(DiskImageScriptEngine* pThis)
{
    ParseCSV_Free(pThis->pParser);
}


#define LOG_ERROR(pTHIS, FORMAT, ...) fprintf(stderr, \
                                       "%s:%d: error: " FORMAT LINE_ENDING, \
                                       pTHIS->pScriptFilename, \
                                       pTHIS->lineNumber, \
                                       __VA_ARGS__)

static void DiskImageScriptEngine_ProcessScriptFile(DiskImageScriptEngine* pThis, 
                                                    DiskImage*              pDiskImage, 
                                                    const char*             pScriptFilename);
static void processScriptFromTextFile(DiskImageScriptEngine* pThis);
static void processNextScriptLine(DiskImageScriptEngine* pThis, char* pScriptLine);
static void processBlockScriptLine(DiskImageScriptEngine* pThis, size_t fieldCount, const char** ppFields);
static void processRWTS16ScriptLine(DiskImageScriptEngine* pThis, size_t fieldCount, const char** ppFields);
static void reportScriptLineException(DiskImageScriptEngine* pThis);
static void closeTextFile(DiskImageScriptEngine* pThis);
__throws void DiskImage_ProcessScriptFile(DiskImage* pThis, const char* pScriptFilename)
{
    DiskImageScriptEngine_ProcessScriptFile(&pThis->script, pThis, pScriptFilename);
}

static void DiskImageScriptEngine_ProcessScriptFile(DiskImageScriptEngine* pThis, 
                                                    DiskImage*               pDiskImage, 
                                                    const char*              pScriptFilename)
{
    __try
    {
        pThis->pScriptFilename = pScriptFilename;
        pThis->pDiskImage = pDiskImage;
        __throwing_func( pThis->pTextFile = TextFile_CreateFromFile(pScriptFilename) );
    }
    __catch
    {
        LOG_ERROR(pThis, "Failed to open %s for parsing.", pScriptFilename);
        __rethrow;
    }
    
    processScriptFromTextFile(pThis);
}

static void processScriptFromTextFile(DiskImageScriptEngine* pThis)
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

static void processNextScriptLine(DiskImageScriptEngine* pThis, char* pScriptLine)
{
    size_t       fieldCount;
    const char** ppFields;
    
    ParseCSV_Parse(pThis->pParser, pScriptLine);
    fieldCount = ParseCSV_FieldCount(pThis->pParser);
    ppFields = ParseCSV_FieldPointers(pThis->pParser);
    if (fieldCount < 1 || ppFields[0][0] == '\0')
    {
        LOG_ERROR(pThis, "%s cannot be blank.", "Script line");
        return;
    }
    
    __try
    {
        if (0 == strcasecmp(ppFields[0], "block"))
            processBlockScriptLine(pThis, fieldCount, ppFields);
        else if (0 == strcasecmp(ppFields[0], "rwts16"))
            processRWTS16ScriptLine(pThis, fieldCount, ppFields);
        else
            LOG_ERROR(pThis, "%s isn't a recognized image insertion type of BLOCK or RWTS16.", ppFields[0]);
    }
    __catch
    {
        reportScriptLineException(pThis);
        __nothrow;
    }
}

static void processBlockScriptLine(DiskImageScriptEngine* pThis, size_t fieldCount, const char** ppFields)
{
    if (fieldCount != 5)
    {
        LOG_ERROR(pThis, 
                  "%s doesn't contain correct fields: BLOCK,objectFilename,objectStartOffset,insertionLength,block",
                  "Line");
        __throw(invalidArgumentException);
    }
    
    __try
    {
        __throwing_func( DiskImage_ReadObjectFile(pThis->pDiskImage, ppFields[1]) );
        pThis->insert.sourceOffset = strtoul(ppFields[2], NULL, 0);
        pThis->insert.length = strtoul(ppFields[3], NULL, 0);
        pThis->insert.type = DISK_IMAGE_INSERTION_BLOCK;
        pThis->insert.block = strtoul(ppFields[4], NULL, 0);
        __throwing_func( DiskImage_InsertObjectFile(pThis->pDiskImage, &pThis->insert) );
    }
    __catch
    {
        __rethrow;
    }
}

static void processRWTS16ScriptLine(DiskImageScriptEngine* pThis, size_t fieldCount, const char** ppFields)
{
    if (fieldCount != 6)
    {
        LOG_ERROR(pThis, 
                  "%s doesn't contain correct fields: RWTS16,objectFilename,objectStartOffset,insertionLength,track,sector",
                  "Line");
        __throw(invalidArgumentException);
    }
    
    __try
    {
        __throwing_func( DiskImage_ReadObjectFile(pThis->pDiskImage, ppFields[1]) );
        pThis->insert.sourceOffset = strtoul(ppFields[2], NULL, 0);
        pThis->insert.length = strtoul(ppFields[3], NULL, 0);
        pThis->insert.type = DISK_IMAGE_INSERTION_RWTS16;
        pThis->insert.track = strtoul(ppFields[4], NULL, 0);
        pThis->insert.sector = strtoul(ppFields[5], NULL, 0);
        __throwing_func( DiskImage_InsertObjectFile(pThis->pDiskImage, &pThis->insert) );
    }
    __catch
    {
        __rethrow;
    }
}

static void reportScriptLineException(DiskImageScriptEngine* pThis)
{
    const char** ppFields = ParseCSV_FieldPointers(pThis->pParser);
    int          exceptionCode = getExceptionCode();
    
    assert ( exceptionCode == fileException || 
             exceptionCode == invalidArgumentException ||
             exceptionCode == blockExceedsImageBoundsException ||
             exceptionCode == invalidInsertionTypeException ||
             exceptionCode == invalidSectorException ||
             exceptionCode == invalidTrackException);

    if (exceptionCode == fileException)
        LOG_ERROR(pThis, "Failed to read '%s' object file.", ppFields[1]);
    else if (exceptionCode == blockExceedsImageBoundsException)
        LOG_ERROR(pThis, "Write starting at block %u won't fit in output image file.", pThis->insert.block);
    else if (exceptionCode == invalidInsertionTypeException)
        LOG_ERROR(pThis, "%s insertion type isn't supported for this output image type.", ppFields[0]);
    else if (exceptionCode == invalidSectorException)
        LOG_ERROR(pThis, "%u specifies an invalid sector.  Must be 0 - 15.", pThis->insert.sector);
    else if (exceptionCode == invalidTrackException)
        LOG_ERROR(pThis, "Write starting at track/sector %u/%u won't fit in output image file.", 
                  pThis->insert.track, pThis->insert.sector);
}

static void closeTextFile(DiskImageScriptEngine* pThis)
{
    TextFile_Free(pThis->pTextFile);
    pThis->pTextFile = NULL;
}


static void DiskImageScriptEngine_ProcessScript(DiskImageScriptEngine* pThis, 
                                                DiskImage*             pDiskImage,
                                                char*                  pScriptText);
__throws void DiskImage_ProcessScript(DiskImage* pThis, char* pScriptText)
{
    DiskImageScriptEngine_ProcessScript(&pThis->script, pThis, pScriptText);
}

static void DiskImageScriptEngine_ProcessScript(DiskImageScriptEngine* pThis, 
                                                DiskImage*             pDiskImage,
                                                char*                  pScriptText)
{
    __try
    {
        pThis->pDiskImage = pDiskImage;
        pThis->pScriptFilename = "<null>";
        __throwing_func( pThis->pTextFile = TextFile_CreateFromString(pScriptText) );
    }
    __catch
    {
        __rethrow;
    }

    processScriptFromTextFile(pThis);
}


static FILE* openFile(const char* pFilename, const char* pMode);
static size_t readFile(void* pBuffer, size_t bytesToRead, FILE* pFile);
static unsigned int roundUpLengthToBlockSize(unsigned int length);
__throws void DiskImage_ReadObjectFile(DiskImage* pThis, const char* pFilename)
{
    FILE*         pFile;
    SavFileHeader header;
    unsigned int  roundedObjectSize;
    
    __try
    {
        __throwing_func( pFile = openFile(pFilename, "r") );
        __throwing_func( readFile(&header, sizeof(header), pFile) );
        roundedObjectSize = roundUpLengthToBlockSize(header.length);
        __throwing_func( ByteBuffer_Allocate(&pThis->object, roundedObjectSize) );
        __throwing_func( ByteBuffer_ReadPartialFromFile(&pThis->object, header.length, pFile) );
        pThis->objectFileLength = header.length;
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
    return (length + (DISK_IMAGE_BLOCK_SIZE - 1)) & ~(DISK_IMAGE_BLOCK_SIZE - 1);
}


static void validateSourceObjectParameters(DiskImage* pThis, DiskImageInsert* pInsert);
__throws void DiskImage_InsertObjectFile(DiskImage* pThis, DiskImageInsert* pInsert)
{
    __try
        validateSourceObjectParameters(pThis, pInsert);
    __catch
        __rethrow;
    
    pThis->pVTable->insertData(pThis, pThis->object.pBuffer, pInsert);
}

static void validateSourceObjectParameters(DiskImage* pThis, DiskImageInsert* pInsert)
{
    if (pInsert->sourceOffset >= pThis->objectFileLength)
        __throw(invalidSourceOffsetException);
    if (pInsert->sourceOffset + pInsert->length > pThis->object.bufferSize)
        __throw(invalidLengthException);
}


__throws void DiskImage_WriteImage(DiskImage* pThis, const char* pImageFilename)
{
    FILE* pFile = NULL;

    __try
    {
        __throwing_func( pFile = openFile(pImageFilename, "w") );
        __throwing_func( ByteBuffer_WriteToFile(&pThis->image, pFile) );
    }
    __catch
    {
        fclose(pFile);
        __rethrow;
    }
    
    fclose(pFile);
}


unsigned char* DiskImage_GetImagePointer(DiskImage* pThis)
{
    return pThis->image.pBuffer;
}


size_t DiskImage_GetImageSize(DiskImage* pThis)
{
    return pThis->image.bufferSize;
}
