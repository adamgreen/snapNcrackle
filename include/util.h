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
#ifndef _UTIL_H_
#define _UTIL_H_

#include <string.h>
#include <stdlib.h>
#include "try_catch.h"

#define ARRAYSIZE(X) (sizeof(X) / sizeof(X[0]))

#define TRUE  1
#define FALSE 0

#define LINE_ENDING "\n"

#define LO_BYTE(X) ((X) & 0xFF)
#define HI_BYTE(X) (((X) >> 8) &0xFF)

static inline void* allocateAndZero(size_t size)
{
    void* pAlloc = malloc(size);
    if (!pAlloc)
        __throw(outOfMemoryException);
    memset(pAlloc, 0, size);
    
    return pAlloc;
}

#endif /* _UTIL_H_ */
