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
#include "LineInfo.h"
#include "LineInfoTest.h"

static void growLineTextBufferIfTooSmall(LineInfo* pThis, size_t lineTextSize);
__throws void LineInfo_Init(LineInfo* pThis)
{
    memset(pThis, 0, sizeof(*pThis));
    __try
        growLineTextBufferIfTooSmall(pThis, 256);
    __catch
        __rethrow;

    return;
}

static void growLineTextBufferIfTooSmall(LineInfo* pThis, size_t lineTextSize)
{
    char* pRealloc = NULL;
    
    if (pThis->lineTextSize >= lineTextSize)
        return;
        
    pRealloc = realloc(pThis->pLineText, lineTextSize);
    if (!pRealloc)
        __throw(outOfMemoryException);
    pThis->pLineText = pRealloc;
    pThis->lineTextSize = lineTextSize;
}


void LineInfo_Free(LineInfo* pThis)
{
    free(pThis->pLineText);
    memset(pThis, 0, sizeof(*pThis));
}


__throws void LineInfo_SaveLineText(LineInfo* pThis, const char* pLineText)
{
    growLineTextBufferIfTooSmall(pThis, strlen(pLineText) + 1);
    strcpy(pThis->pLineText, pLineText);
}
