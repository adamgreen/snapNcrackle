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
#ifndef _BYTE_BUFFER_H_
#define _BYTE_BUFFER_H_

#include <stdio.h>
#include "try_catch.h"

typedef struct ByteBuffer
{
    unsigned char* pBuffer;
    unsigned int   bufferSize;
} ByteBuffer;


__throws void ByteBuffer_Allocate(ByteBuffer* pThis, unsigned int bufferSize);
         void ByteBuffer_Free(ByteBuffer* pThis);

__throws void ByteBuffer_WriteToFile(ByteBuffer* pThis, FILE* pFile);
__throws void ByteBuffer_ReadFromFile(ByteBuffer* pThis, FILE* pFile);

#endif /* _BYTE_BUFFER_H_ */
