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
#include "ListFile.h"
#include "ListFileTest.h"

struct ListFile
{
    FILE*   pFile;
    char*   pLineText;
    size_t  lineTextSize;
};


static ListFile* allocateAndZeroObject(void);
static void growLineTextBufferIfNecessary(ListFile* pThis, size_t lineTextSize);
__throws ListFile* ListFile_Create(FILE* pOutputFile)
{
    ListFile* pThis = NULL;
    
    __try
    {
        __throwing_func( pThis = allocateAndZeroObject() );
        __throwing_func( growLineTextBufferIfNecessary(pThis, 256) );
    }
    __catch
    {
        ListFile_Free(pThis);
        __rethrow_and_return(NULL);
    }
    
    return pThis;
}

static ListFile* allocateAndZeroObject(void)
{
    ListFile* pThis = NULL;
    
    pThis = malloc(sizeof(*pThis));
    if (!pThis)
        __throw_and_return(outOfMemoryException, NULL);
    
    memset(pThis, 0, sizeof(*pThis));
    return pThis;
}

static void growLineTextBufferIfNecessary(ListFile* pThis, size_t lineTextSize)
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

void ListFile_Free(ListFile* pThis)
{
    if (!pThis)
        return;
    
    free(pThis->pLineText);
    free(pThis);
}

__throws void ListFile_SaveLineText(ListFile* pThis, const char* pLineText)
{
    growLineTextBufferIfNecessary(pThis, strlen(pLineText) + 1);
    strcpy(pThis->pLineText, pLineText);
}

__throws void ListFile_OutputLine(ListFile* pThis)
{
    fprintf(stdout, "%04X: %02X %02X % 4s % 5d %s\n", 0, 0, 0, "", 1, pThis->pLineText);
}
