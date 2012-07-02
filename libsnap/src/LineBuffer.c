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
#include "LineBuffer.h"
#include "LineBufferTest.h"

struct LineBuffer
{
    char*          pText;
    size_t         textSize;
};


static LineBuffer* allocAndZero(void);
static void growLineTextBufferIfTooSmall(LineBuffer* pThis, size_t textSize);
__throws LineBuffer* LineBuffer_Create(void)
{
    LineBuffer* pThis = NULL;
    
    __try
    {
        __throwing_func( pThis = allocAndZero() );
        __throwing_func( growLineTextBufferIfTooSmall(pThis, 256) );
        pThis->pText[0] = '\0';
    }
    __catch
    {
        LineBuffer_Free(pThis);
        __rethrow_and_return(NULL);
    }

    return pThis;
}

static LineBuffer* allocAndZero(void)
{
    LineBuffer* pThis = malloc(sizeof(*pThis));
    if (!pThis)
        __throw_and_return(outOfMemoryException, NULL);
    memset(pThis, 0, sizeof(*pThis));
    return pThis;
}

static void growLineTextBufferIfTooSmall(LineBuffer* pThis, size_t textSize)
{
    char* pRealloc = NULL;
    
    if (pThis->textSize >= textSize)
        return;
        
    pRealloc = realloc(pThis->pText, textSize);
    if (!pRealloc)
        __throw(outOfMemoryException);
    pThis->pText = pRealloc;
    pThis->textSize = textSize;
}


void LineBuffer_Free(LineBuffer* pThis)
{
    if (!pThis)
        return;
        
    free(pThis->pText);
    free(pThis);
}


__throws void LineBuffer_Set(LineBuffer* pThis, const char* pText)
{
    __try
        growLineTextBufferIfTooSmall(pThis, strlen(pText) + 1);
    __catch
        __rethrow;
    strcpy(pThis->pText, pText);
}


const char* LineBuffer_Get(LineBuffer* pThis)
{
    return pThis->pText;
}
