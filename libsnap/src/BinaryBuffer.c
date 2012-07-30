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
#include <string.h>
#include "BinaryBuffer.h"
#include "BinaryBufferTest.h"
#include "util.h"


typedef struct FileWriteEntry
{
    struct FileWriteEntry* pNext;
    unsigned char*         pBase;
    size_t                 length;
    unsigned short         baseAddress;
    char                   filename[256];
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
    BinaryBuffer* pThis;
    
    __try
        pThis = allocateAndZero(sizeof(*pThis));
    __catch
        __rethrow_and_return(NULL);

    pThis->pBuffer = malloc(bufferSize);
    if (!pThis->pBuffer)
    {
        BinaryBuffer_Free(pThis);
        __throw_and_return(outOfMemoryException, NULL);
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
        __throw_and_return(outOfMemoryException, NULL);

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
        __throw_and_return(invalidArgumentException, NULL);
        
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


static void initializeFileWriteEntry(BinaryBuffer* pThis, FileWriteEntry* pEntry, const char* pFilename);
static void addFileWriteEntryToList(BinaryBuffer* pThis, FileWriteEntry* pEntry);
__throws void BinaryBuffer_QueueWriteToFile(BinaryBuffer* pThis, const char* pFilename)
{
    FileWriteEntry* pEntry = NULL;
    
    __try
    {
        __throwing_func( pEntry = allocateAndZero(sizeof(*pEntry)) );
        __throwing_func( initializeFileWriteEntry(pThis, pEntry, pFilename) );
        addFileWriteEntryToList(pThis, pEntry);
    }
    __catch
    {
        free(pEntry);
        __rethrow;
    }
}

static void initializeFileWriteEntry(BinaryBuffer* pThis, FileWriteEntry* pEntry, const char* pFilename)
{
    if (strlen(pFilename) > sizeof(pEntry->filename)-1)
        __throw(invalidArgumentException);

    strcpy(pEntry->filename, pFilename);
    pEntry->baseAddress = pThis->baseAddress;
    pEntry->pBase = pThis->pBase;
    pEntry->length = pThis->pCurrent - pThis->pBase;
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
    static const unsigned char signature[4] = BINARY_BUFFER_SAV_SIGNATURE;
    SavFileHeader              header;
    size_t                     bytesWritten;
    FILE*                      pFile;
    
    memcpy(header.signature, signature, sizeof(header.signature));
    header.address = pEntry->baseAddress;
    header.length = pEntry->length;

    pFile = fopen(pEntry->filename, "w");
    if (!pFile)
        __throw(fileException);
    
    bytesWritten = fwrite(&header, 1, sizeof(header), pFile);
    bytesWritten += fwrite(pEntry->pBase, 1, pEntry->length, pFile);
    fclose(pFile);
    if (bytesWritten != pEntry->length + sizeof(header))
        __throw(fileException);
}
