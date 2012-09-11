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
#include "util.h"

struct ListFile
{
    FILE*          pFile;
    unsigned char* pMachineCode;
    size_t         machineCodeSize;
    int            flags;
    unsigned short address;
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
        __rethrow;
    }
    pThis->pFile = pOutputFile;
    
    return pThis;
}

static ListFile* allocateAndZeroObject(void)
{
    ListFile* pThis = NULL;
    
    pThis = malloc(sizeof(*pThis));
    if (!pThis)
        __throw(outOfMemoryException);
    
    memset(pThis, 0, sizeof(*pThis));
    return pThis;
}


void ListFile_Free(ListFile* pThis)
{
    if (!pThis)
        return;
    
    free(pThis);
}


static void initMachineCodeFields(ListFile* pThis, LineInfo* pLineInfo);
static void fillAddressBuffer(LineInfo* pLineInfo, char* pOutputBuffer);
static void fillMachineCodeOrSymbolBuffer(ListFile* pThis, LineInfo* pLineInfo, char* pOutputBuffer);
static void fillMachineCodeBuffer(ListFile* pThis, char* pOutputBuffer);
static void listOverflowMachineCodeLine(ListFile* pThis);
void ListFile_OutputLine(ListFile* pThis, LineInfo* pLineInfo)
{
    char           addressString[4+1] = "    ";
    char           machineCodeOrSymbol[8+1] = "        ";
    
    initMachineCodeFields(pThis, pLineInfo);
    fillAddressBuffer(pLineInfo, addressString);
    fillMachineCodeOrSymbolBuffer(pThis, pLineInfo, machineCodeOrSymbol);
    fprintf(pThis->pFile, "%4s: %8s %*s% 5d %s" LINE_ENDING, 
            addressString,
            machineCodeOrSymbol,
            pLineInfo->indentation, "",
            pLineInfo->lineNumber, 
            pLineInfo->pLineText);
            
    while (pThis->machineCodeSize > 0)
        listOverflowMachineCodeLine(pThis);
}

static void initMachineCodeFields(ListFile* pThis, LineInfo* pLineInfo)
{
    pThis->address = pLineInfo->address;
    pThis->pMachineCode = pLineInfo->pMachineCode;
    pThis->machineCodeSize = pLineInfo->machineCodeSize;
    pThis->flags = pLineInfo->flags;
}

static void fillAddressBuffer(LineInfo* pLineInfo, char* pOutputBuffer)
{
    if (pLineInfo->machineCodeSize > 0)
        sprintf(pOutputBuffer, "%04X", pLineInfo->address);
}

static void fillMachineCodeOrSymbolBuffer(ListFile* pThis, LineInfo* pLineInfo, char* pOutputBuffer)
{
    if (pLineInfo->flags & LINEINFO_FLAG_WAS_EQU)
        sprintf(pOutputBuffer, "   =%04X", pLineInfo->pSymbol->expression.value);
    else if (pLineInfo->machineCodeSize > 0)
        fillMachineCodeBuffer(pThis, pOutputBuffer);
}

static void fillMachineCodeBuffer(ListFile* pThis, char* pOutputBuffer)
{
    size_t bytesUsed = 0;
    
    if (pThis->machineCodeSize == 1)
    {
        sprintf(pOutputBuffer, "%02X      ", pThis->pMachineCode[0]);
        bytesUsed = 1;
    }
    else if (pThis->machineCodeSize == 2)
    {
        sprintf(pOutputBuffer, "%02X %02X   ", pThis->pMachineCode[0], pThis->pMachineCode[1]);
        bytesUsed = 2;
    }
    else if (pThis->machineCodeSize >= 3)
    {
        sprintf(pOutputBuffer, "%02X %02X %02X", pThis->pMachineCode[0], pThis->pMachineCode[1], pThis->pMachineCode[2]);
        bytesUsed = 3;
    }

    pThis->pMachineCode += bytesUsed;
    pThis->machineCodeSize -= bytesUsed;
}

static void listOverflowMachineCodeLine(ListFile* pThis)
{
    char machineCodeBuffer[8+1] = "        ";

    pThis->address += 3;
    fillMachineCodeBuffer(pThis, machineCodeBuffer);
    fprintf(pThis->pFile, "%04X: %8s" LINE_ENDING, 
            pThis->address,
            machineCodeBuffer);
}
