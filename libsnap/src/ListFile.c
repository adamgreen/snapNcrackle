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
    FILE*    pFile;
};


static ListFile* allocateAndZeroObject(void);
__throws ListFile* ListFile_Create(FILE* pOutputFile)
{
    ListFile* pThis = NULL;
    
    __try
    {
        pThis = allocateAndZeroObject();
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


void ListFile_Free(ListFile* pThis)
{
    if (!pThis)
        return;
    
    free(pThis);
}


void ListFile_OutputLine(ListFile* pThis, LineInfo* pLineInfo)
{
    char address[4+1] = "    ";
    char machineCode1[2+1] = "  ";
    char machineCode2[2+1] = "  ";
    char symbolValue[4+1] = "";
    
    fprintf(stdout, "%4s: %2s %2s % 4s % 5d %s\n", 
            address,
            machineCode1,
            machineCode2,
            symbolValue, 
            pLineInfo->lineNumber, 
            pLineInfo->pLineText);
}
