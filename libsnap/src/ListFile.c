/*  Copyright (C) 2013  Adam Green (https://github.com/adamgreen)

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

__throws ListFile* ListFile_Create(FILE* pOutputFile)
{
    ListFile* pThis = NULL;
    
    pThis = allocateAndZero(sizeof(*pThis));
    pThis->pFile = pOutputFile;
    
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
    char           machineCodeOrSymbol[2+1+2+1+2+1] = "        ";
    
    initMachineCodeFields(pThis, pLineInfo);
    fillAddressBuffer(pLineInfo, addressString);
    fillMachineCodeOrSymbolBuffer(pThis, pLineInfo, machineCodeOrSymbol);
    fprintf(pThis->pFile, "%4s: %8s %*s% 5d %.*s" LINE_ENDING, 
            addressString,
            machineCodeOrSymbol,
            pLineInfo->indentation, "",
            pLineInfo->lineNumber, 
            pLineInfo->lineText.stringLength, pLineInfo->lineText.pString);
            
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
    
    switch(pThis->machineCodeSize)
    {
    case 1:
        sprintf(pOutputBuffer, "%02X      ", pThis->pMachineCode[0]);
        bytesUsed = 1;
        break;
    case 2:
        sprintf(pOutputBuffer, "%02X %02X   ", pThis->pMachineCode[0], pThis->pMachineCode[1]);
        bytesUsed = 2;
        break;
    default:
        sprintf(pOutputBuffer, "%02X %02X %02X", pThis->pMachineCode[0], pThis->pMachineCode[1], pThis->pMachineCode[2]);
        bytesUsed = 3;
        break;
    }

    pThis->pMachineCode += bytesUsed;
    pThis->machineCodeSize -= bytesUsed;
}

static void listOverflowMachineCodeLine(ListFile* pThis)
{
    char machineCodeBuffer[2+1+2+1+2+1] = "        ";

    pThis->address += 3;
    fillMachineCodeBuffer(pThis, machineCodeBuffer);
    fprintf(pThis->pFile, "%04X: %8s" LINE_ENDING, 
            pThis->address,
            machineCodeBuffer);
}
