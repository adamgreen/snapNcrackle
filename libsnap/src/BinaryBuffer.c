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


struct BinaryBuffer
{
    unsigned char* pBuffer;
    unsigned char* pEnd;
    unsigned char* pCurrent;
    size_t         bufferSize;
};

__throws BinaryBuffer* BinaryBuffer_Create(size_t bufferSize)
{
    BinaryBuffer* pThis = malloc(sizeof(*pThis));
    if (!pThis)
        __throw_and_return(outOfMemoryException, NULL);
    memset(pThis, 0, sizeof(*pThis));

    pThis->bufferSize = bufferSize;
    pThis->pBuffer = malloc(bufferSize);
    if (!pThis->pBuffer)
    {
        BinaryBuffer_Free(pThis);
        __throw_and_return(outOfMemoryException, NULL);
    }
    pThis->pCurrent = pThis->pBuffer;
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


__throws unsigned char* BinaryBuffer_Allocate(BinaryBuffer* pThis, size_t bytesToAllocate)
{
    size_t         bytesLeft = pThis->pEnd - pThis->pCurrent;
    unsigned char* pAlloc = pThis->pCurrent;
    
    if (bytesLeft < bytesToAllocate)
        __throw_and_return(outOfMemoryException, NULL);

    pThis->pCurrent += bytesToAllocate;
    return pAlloc;
}


__throws void BinaryBuffer_WriteToFile(BinaryBuffer* pThis, const char* pFilename)
{
    size_t bytesToWrite = pThis->pEnd - pThis->pBuffer;
    size_t bytesWritten;
    FILE*  pFile;
    
    pFile = fopen(pFilename, "w");
    if (!pFile)
        __throw(fileException);
        
    bytesWritten = fwrite(pThis->pBuffer, 1, bytesToWrite, pFile);
    fclose(pFile);
    if (bytesWritten != bytesToWrite)
        __throw(fileException);
}
