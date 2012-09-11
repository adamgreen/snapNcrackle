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
#ifndef _BINARY_BUFFER_H_
#define _BINARY_BUFFER_H_

#include "try_catch.h"
#include "SizedString.h"


#define BINARY_BUFFER_SAV_SIGNATURE "SAV\x1a"


typedef struct SavFileHeader
{
    char           signature[4];
    unsigned short address;
    unsigned short length;
} SavFileHeader;


typedef struct BinaryBuffer BinaryBuffer;


__throws BinaryBuffer* BinaryBuffer_Create(size_t bufferSize);
         void          BinaryBuffer_Free(BinaryBuffer* pThis);
         
__throws unsigned char* BinaryBuffer_Alloc(BinaryBuffer* pThis, size_t bytesToAllocate);
__throws unsigned char* BinaryBuffer_Realloc(BinaryBuffer* pThis, unsigned char* pToRealloc, size_t bytesToAllocate);
         void           BinaryBuffer_FailAllocation(BinaryBuffer* pThis, size_t allocationToFail);
         
         void           BinaryBuffer_SetOrigin(BinaryBuffer* pThis, unsigned short origin);
         unsigned short BinaryBuffer_GetOrigin(BinaryBuffer* pThis);
__throws void           BinaryBuffer_QueueWriteToFile(BinaryBuffer* pThis, 
                                                      const char*   pDirectory, 
                                                      SizedString*  pFilename);
__throws void           BinaryBuffer_ProcessWriteFileQueue(BinaryBuffer* pThis);

#endif /* _BINARY_BUFFER_H_ */
