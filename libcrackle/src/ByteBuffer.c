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
#include "ByteBuffer.h"
#include "ByteBufferTest.h"
#include "util.h"


__throws void ByteBuffer_Allocate(ByteBuffer* pThis, unsigned int bufferSize)
{
    __try
    {
        ByteBuffer_Free(pThis);
        pThis->pBuffer = allocateAndZero(bufferSize);
        pThis->bufferSize = bufferSize;
    }
    __catch
    {
        ByteBuffer_Free(pThis);
        __rethrow;
    }
}


void ByteBuffer_Free(ByteBuffer* pThis)
{
    if (!pThis)
        return;
    
    free(pThis->pBuffer);
    memset(pThis, 0, sizeof(*pThis));
}


__throws void ByteBuffer_WriteToFile(ByteBuffer* pThis, FILE* pFile)
{
    size_t bytesWritten = fwrite(pThis->pBuffer, 1, pThis->bufferSize, pFile);
    if (bytesWritten != pThis->bufferSize)
        __throw(fileException);
}


__throws void ByteBuffer_ReadFromFile(ByteBuffer* pThis, FILE* pFile)
{
    ByteBuffer_ReadPartialFromFile(pThis, pThis->bufferSize, pFile);
}


__throws void ByteBuffer_ReadPartialFromFile(ByteBuffer* pThis, unsigned int bytesToRead, FILE* pFile)
{
    size_t bytesRead;
    
    if (bytesToRead > pThis->bufferSize)
        __throw(invalidArgumentException);
        
    bytesRead = fread(pThis->pBuffer, 1, bytesToRead, pFile);
    if (bytesRead != bytesToRead)
        __throw(fileException);
}
