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

#define LINE_ENDING "\n"

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
    pThis->pFile = pOutputFile;
    
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
    char machineCodeOrSymbol[8+1] = "        ";
    
    if (pLineInfo->machineCodeSize > 0)
        sprintf(address, "%04X", pLineInfo->address);

    if (pLineInfo->validSymbol)
        sprintf(machineCodeOrSymbol, "   =%04X", pLineInfo->symbolValue);
    else if (pLineInfo->machineCodeSize == 1)
        sprintf(machineCodeOrSymbol, "%02X      ", pLineInfo->machineCode[0]);
    else if (pLineInfo->machineCodeSize == 2)
        sprintf(machineCodeOrSymbol, "%02X %02X   ", pLineInfo->machineCode[0], pLineInfo->machineCode[1]);
    else if (pLineInfo->machineCodeSize == 3)
        sprintf(machineCodeOrSymbol, "%02X %02X %02X", pLineInfo->machineCode[0], pLineInfo->machineCode[1], pLineInfo->machineCode[2]);
    
    fprintf(pThis->pFile, "%4s: %8s % 5d %s" LINE_ENDING, 
            address,
            machineCodeOrSymbol,
            pLineInfo->lineNumber, 
            pLineInfo->pLineText);
}
