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
#include "SizedString.h"
#include "BinaryBuffer.h"
#include "ParseCSV.h"
#include "util.h"


#define NUMBER_OF_SYMBOL_TABLE_HASH_BUCKETS 511
#define SIZE_OF_OBJECT_AND_DUMMY_BUFFERS    (64 * 1024)


typedef struct OpCodeEntry
{
    const char* pOperator;
    void (*directiveHandler)(Assembler *pThis);
    unsigned char opcodeImmediate;
    unsigned char opcodeAbsolute;
    unsigned char opcodeZeroPage;
    unsigned char opcodeImplied;
    unsigned char opcodeZeroPageIndexedIndirect;
    unsigned char opcodeIndirectIndexed;
    unsigned char opcodeZeroPageIndexedX;
    unsigned char opcodeZeroPageIndexedY;
    unsigned char opcodeAbsoluteIndexedX;
    unsigned char opcodeAbsoluteIndexedY;
    unsigned char opcodeRelative;
    unsigned char opcodeAbsoluteIndirect;
    unsigned char opcodeAbsoluteIndexedIndirect;
    unsigned char opcodeZeroPageIndirect;
} OpCodeEntry;


typedef struct TextFileNode
{
    TextFile*            pTextFile;
    struct TextFileNode* pNext;
} TextFileNode;


struct Assembler
{
    TextFile*                  pTextFile;
    TextFile*                  pMainFile;
    TextFileNode*              pIncludedFiles;
    SymbolTable*               pSymbols;
    const AssemblerInitParams* pInitParams;
    ListFile*                  pListFile;
    FILE*                      pFileForListing;
    char*                      pPutDirectories;
    ParseCSV*                  pPutSearchPath;
    LineInfo*                  pLineInfo;
    SizedString                globalLabel;
    BinaryBuffer*              pObjectBuffer;
    BinaryBuffer*              pDummyBuffer;
    BinaryBuffer*              pCurrentBuffer;
    OpCodeEntry*               instructionSets[INSTRUCTION_SET_INVALID];
    size_t                     instructionSetSizes[INSTRUCTION_SET_INVALID];
    ParsedLine                 parsedLine;
    LineInfo                   linesHead;
    InstructionSetSupported    instructionSet;
    unsigned int               errorCount;
    int                        indentation;
    unsigned short             programCounter;
    unsigned short             programCounterBeforeDUM;
};


#define LOG_ERROR(pASSEMBLER, FORMAT, ...) fprintf(stderr, \
                                       "%s:%d: error: " FORMAT LINE_ENDING, \
                                       TextFile_GetFilename(pASSEMBLER->pLineInfo->pTextFile), \
                                       pASSEMBLER->pLineInfo->lineNumber, \
                                       __VA_ARGS__), pASSEMBLER->errorCount++


__throws Symbol* Assembler_FindLabel(Assembler* pThis, SizedString* pLabelName);

#endif /* _ASSEMBLER_PRIV_H_ */
