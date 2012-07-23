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
#ifndef _ASSEMBLER_PRIV_H_
#define _ASSEMBLER_PRIV_H_

#include <stdio.h>
#include "Assembler.h"
#include "AssemblerTest.h"
#include "TextFile.h"
#include "SymbolTable.h"
#include "ParseLine.h"
#include "ListFile.h"
#include "LineBuffer.h"
#include "BinaryBuffer.h"
#include "util.h"


#define NUMBER_OF_SYMBOL_TABLE_HASH_BUCKETS 511
#define SIZE_OF_OBJECT_AND_DUMMY_BUFFERS    (64 * 1024)


struct Assembler
{
    const char*    pSourceFilename;
    TextFile*      pTextFile;
    SymbolTable*   pSymbols;
    ListFile*      pListFile;
    LineBuffer*    pLineText;
    char*          pLocalLabelStart;
    LineInfo*      pLineInfo;
    BinaryBuffer*  pObjectBuffer;
    BinaryBuffer*  pDummyBuffer;
    BinaryBuffer*  pCurrentBuffer;
    size_t         maxLocalLabelSize;
    ParsedLine     parsedLine;
    LineInfo       linesHead;
    unsigned int   errorCount;
    unsigned short programCounter;
    unsigned short programCounterBeforeDUM;
    char           labelBuffer[256];
};


#define LOG_ERROR(pASSEMBLER, FORMAT, ...) fprintf(stderr, \
                                       "%s:%d: error: " FORMAT LINE_ENDING, \
                                       pASSEMBLER->pSourceFilename, \
                                       pASSEMBLER->pLineInfo->lineNumber, \
                                       __VA_ARGS__), pASSEMBLER->errorCount++


__throws Symbol* Assembler_FindLabel(Assembler* pThis, const char* pLabelName, size_t labelLength);

#endif /* _ASSEMBLER_PRIV_H_ */
