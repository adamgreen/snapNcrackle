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
#include <string.h>
#include "BinaryBuffer.h"
#include "BinaryBufferTest.h"
#include "util.h"


typedef struct FileWriteEntry
{
    struct FileWriteEntry* pNext;
    unsigned char*         pBase;
    size_t                 contentLength;
    size_t                 headerLength;
    union
    {
        SavFileHeader     savFileHeader;
        RW18SavFileHeader rw18FileHeader;
    };
    unsigned short         baseAddress;
    char                   filename[PATH_LENGTH];
} FileWriteEntry;


struct BinaryBuffer
{
    unsigned char*  pBuffer;
    unsigned char*  pEnd;
    unsigned char*  pCurrent;
    unsigned char*  pLastAlloc;
    unsigned char*  pBase;
    FileWriteEntry* pFileWriteHead;
    FileWriteEntry* pFileWriteTail;
    size_t          allocationToFail;
    unsigned short  baseAddress;
};

static void* allocateAndZero(size_t sizeToAllocate);
__throws BinaryBuffer* BinaryBuffer_Create(size_t bufferSize)
{
    BinaryBuffer* pThis = allocateAndZero(sizeof(*pThis));
    pThis->pBuffer = malloc(bufferSize);
    if (!pThis->pBuffer)
    {
        BinaryBuffer_Free(pThis);
        __throw(outOfMemoryException);
    }
    pThis->pCurrent = pThis->pBuffer;
    pThis->pBase = pThis->pBuffer;
    pThis->pEnd = pThis->pCurrent + bufferSize;
    
    return pThis;
}


static void freeFileWriteEntries(BinaryBuffer* pThis);
void BinaryBuffer_Free(BinaryBuffer* pThis)
{
    if (!pThis)
        return;
        
    freeFileWriteEntries(pThis);
    free(pThis->pBuffer);
    free(pThis);
}

static void freeFileWriteEntries(BinaryBuffer* pThis)
{
    FileWriteEntry* pEntry = pThis->pFileWriteHead;
    
    while (pEntry)
    {
        FileWriteEntry* pNext = pEntry->pNext;
        free(pEntry);
        pEntry = pNext;
    }
}


static int shouldInjectFailureOnThisAllocation(BinaryBuffer* pThis);
__throws unsigned char* BinaryBuffer_Alloc(BinaryBuffer* pThis, size_t bytesToAllocate)
{
    size_t         bytesLeft = pThis->pEnd - pThis->pCurrent;
    unsigned char* pAlloc = pThis->pCurrent;
    
    if (bytesLeft < bytesToAllocate || shouldInjectFailureOnThisAllocation(pThis))
        __throw(outOfMemoryException);

    pThis->pCurrent += bytesToAllocate;
    pThis->pLastAlloc = pAlloc;
    
    return pAlloc;
}

static int shouldInjectFailureOnThisAllocation(BinaryBuffer* pThis)
{
    if (pThis->allocationToFail == 0)
        return FALSE;
    
    if (--pThis->allocationToFail == 0)
        return TRUE;
        
    return FALSE;
}


__throws unsigned char* BinaryBuffer_Realloc(BinaryBuffer* pThis, unsigned char* pToRealloc, size_t bytesToAllocate)
{
    if (!pToRealloc)
        return BinaryBuffer_Alloc(pThis, bytesToAllocate);
    
    if(pToRealloc != pThis->pLastAlloc)
        __throw(invalidArgumentException);
        
    pThis->pCurrent = pThis->pLastAlloc;
    return BinaryBuffer_Alloc(pThis, bytesToAllocate);
}


void BinaryBuffer_FailAllocation(BinaryBuffer* pThis, size_t allocationToFail)
{
    pThis->allocationToFail = allocationToFail;
}


void BinaryBuffer_SetOrigin(BinaryBuffer* pThis, unsigned short origin)
{
    pThis->baseAddress = origin;
    pThis->pBase = pThis->pCurrent;
}


unsigned short BinaryBuffer_GetOrigin(BinaryBuffer* pThis)
{
    return pThis->baseAddress;
}


static FileWriteEntry* queueWriteToFile(BinaryBuffer* pThis, 
                                        const char*   pDirectoryName, 
                                        SizedString*  pFilename,
                                        const char*   pFilenameSuffix);
static void initializeBaseFileWriteEntry(BinaryBuffer*   pThis, 
                                         FileWriteEntry* pEntry, 
                                         const char*     pDirectoryName,
                                         SizedString*    pFilename,
                                         const char*     pFilenameSuffix);
static void addFileWriteEntryToList(BinaryBuffer* pThis, FileWriteEntry* pEntry);
__throws void BinaryBuffer_QueueWriteToFile(BinaryBuffer* pThis, 
                                            const char*   pDirectoryName, 
                                            SizedString*  pFilename, 
                                            const char*   pFilenameSuffix)
{
    static const unsigned char signature[4] = BINARY_BUFFER_SAV_SIGNATURE;
    FileWriteEntry*            pEntry = NULL;
    
    __try
    {
        pEntry = queueWriteToFile(pThis, pDirectoryName, pFilename, pFilenameSuffix);
        memcpy(pEntry->savFileHeader.signature, signature, sizeof(pEntry->savFileHeader.signature));
        pEntry->savFileHeader.address = pThis->baseAddress;
        pEntry->savFileHeader.length = pEntry->contentLength;
        pEntry->headerLength = sizeof(pEntry->savFileHeader);
    }
    __catch
    {
        __rethrow;
    }
}

static FileWriteEntry* queueWriteToFile(BinaryBuffer* pThis, 
                                        const char*   pDirectoryName, 
                                        SizedString*  pFilename,
                                        const char*   pFilenameSuffix)
{
    FileWriteEntry* pEntry = NULL;
    
    __try
    {
        pEntry = allocateAndZero(sizeof(*pEntry));
        initializeBaseFileWriteEntry(pThis, pEntry, pDirectoryName, pFilename, pFilenameSuffix);
        addFileWriteEntryToList(pThis, pEntry);
    }
    __catch
    {
        free(pEntry);
        __rethrow;
    }
    
    return pEntry;
}

static void initializeBaseFileWriteEntry(BinaryBuffer*   pThis, 
                                         FileWriteEntry* pEntry, 
                                         const char*     pDirectoryName,
                                         SizedString*    pFilename,
                                         const char*     pFilenameSuffix)
{
    static const char pathSeparator = PATH_SEPARATOR;
    size_t filenameLength = SizedString_strlen(pFilename);
    size_t directoryLength = pDirectoryName ? strlen(pDirectoryName) : 0;
    size_t slashSpace = !pDirectoryName ? 0 : (pDirectoryName[directoryLength-1] == PATH_SEPARATOR ? 0 : 1);
    size_t suffixLength = pFilenameSuffix ? strlen(pFilenameSuffix) : 0;
    size_t fullLength = directoryLength + slashSpace + filenameLength + suffixLength;
    
    if (fullLength > sizeof(pEntry->filename)-1)
        __throw(invalidArgumentException);

    memcpy(pEntry->filename, pDirectoryName, directoryLength);
    memcpy(pEntry->filename + directoryLength, &pathSeparator, slashSpace);
    memcpy(pEntry->filename + directoryLength + slashSpace, pFilename->pString, filenameLength);
    memcpy(pEntry->filename + directoryLength + slashSpace + filenameLength, pFilenameSuffix, suffixLength);
    pEntry->filename[fullLength] = '\0';
    pEntry->baseAddress = pThis->baseAddress;
    pEntry->pBase = pThis->pBase;
    pEntry->contentLength = pThis->pCurrent - pThis->pBase;
}

static void addFileWriteEntryToList(BinaryBuffer* pThis, FileWriteEntry* pEntry)
{
    if (!pThis->pFileWriteTail)
    {
        pThis->pFileWriteHead = pEntry;
        pThis->pFileWriteTail = pEntry;
    }
    else
    {
        pThis->pFileWriteTail->pNext = pEntry;
        pThis->pFileWriteTail = pEntry;
    }
}


__throws void BinaryBuffer_QueueRW18WriteToFile(BinaryBuffer*  pThis, 
                                                const char*    pDirectoryName, 
                                                SizedString*   pFilename,
                                                const char*    pFilenameSuffix,
                                                unsigned short side,
                                                unsigned short track,
                                                unsigned short offset)
{
    static const unsigned char signature[4] = BINARY_BUFFER_RW18SAV_SIGNATURE;
    FileWriteEntry*            pEntry = NULL;
    
    pEntry = queueWriteToFile(pThis, pDirectoryName, pFilename, pFilenameSuffix);
    memcpy(pEntry->rw18FileHeader.signature, signature, sizeof(pEntry->rw18FileHeader.signature));
    pEntry->rw18FileHeader.side = side;
    pEntry->rw18FileHeader.track = track;
    pEntry->rw18FileHeader.offset = offset;
    pEntry->rw18FileHeader.length = pEntry->contentLength;
    pEntry->headerLength = sizeof(pEntry->rw18FileHeader);
}


static void writeEntryToDisk(FileWriteEntry* pEntry);
__throws void BinaryBuffer_ProcessWriteFileQueue(BinaryBuffer* pThis)
{
    FileWriteEntry* pEntry = pThis->pFileWriteHead;
    int             exceptionThrown = noException;
    
    while (pEntry)
    {
        __try
            writeEntryToDisk(pEntry);
        __catch
            exceptionThrown = getExceptionCode();
        pEntry = pEntry->pNext;
    }
    
    if (exceptionThrown != noException)
        __throw(exceptionThrown);
}

static void writeEntryToDisk(FileWriteEntry* pEntry)
{
    size_t                     bytesWritten;
    FILE*                      pFile;
    
    pFile = fopen(pEntry->filename, "wb");
    if (!pFile)
        __throw(fileException);
    
    bytesWritten = fwrite(&pEntry->savFileHeader, 1, pEntry->headerLength, pFile);
    bytesWritten += fwrite(pEntry->pBase, 1, pEntry->contentLength, pFile);
    fclose(pFile);
    if (bytesWritten != pEntry->contentLength + pEntry->headerLength)
        __throw(fileException);
}
