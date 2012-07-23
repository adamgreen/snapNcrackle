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


struct BinaryBuffer
{
    unsigned char* pBuffer;
    unsigned char* pEnd;
    unsigned char* pCurrent;
    unsigned char* pLastAlloc;
    unsigned char* pBase;
    size_t         allocationToFail;
    unsigned short baseAddress;
};

__throws BinaryBuffer* BinaryBuffer_Create(size_t bufferSize)
{
    BinaryBuffer* pThis = malloc(sizeof(*pThis));
    if (!pThis)
        __throw_and_return(outOfMemoryException, NULL);
    memset(pThis, 0, sizeof(*pThis));

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


void BinaryBuffer_Free(BinaryBuffer* pThis)
{
    if (!pThis)
        return;
        
    free(pThis->pBuffer);
    free(pThis);
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

__throws void BinaryBuffer_WriteToFile(BinaryBuffer* pThis, const char* pFilename)
{
    size_t bytesToWrite = pThis->pCurrent - pThis->pBase;
    size_t bytesWritten;
    FILE*  pFile;
    
    pFile = fopen(pFilename, "w");
    if (!pFile)
        __throw(fileException);
        
    bytesWritten = fwrite(pThis->pBase, 1, bytesToWrite, pFile);
    fclose(pFile);
    if (bytesWritten != bytesToWrite)
        __throw(fileException);
}
