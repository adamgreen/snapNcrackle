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
#include <assert.h>
#include "AssemblerPriv.h"
#include "ExpressionEval.h"
#include "AddressingMode.h"


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


static Assembler* allocateAndZeroObject(void);
static void commonObjectInit(Assembler* pThis);
__throws Assembler* Assembler_CreateFromString(char* pText)
{
    Assembler* pThis = NULL;
    
    __try
    {
        __throwing_func( pThis = allocateAndZeroObject() );
        __throwing_func( commonObjectInit(pThis) );
        __throwing_func( pThis->pTextFile = TextFile_CreateFromString(pText) );
        pThis->pSourceFilename = "filename";
    }
    __catch
    {
        Assembler_Free(pThis);
        __rethrow_and_return(NULL);
    }

    return pThis;
}

static Assembler* allocateAndZeroObject(void)
{
    Assembler* pThis = malloc(sizeof(*pThis));
    if (!pThis)
        __throw_and_return(outOfMemoryException, NULL);
    memset(pThis, 0, sizeof(*pThis));
    
    return pThis;
}

static void commonObjectInit(Assembler* pThis)
{
    memset(pThis, 0, sizeof(*pThis));
    __try
    {
        // UNDONE: Take list file location as parameter.
        __throwing_func( pThis->pListFile = ListFile_Create(stdout) );
        __throwing_func( pThis->pLineText = LineBuffer_Create() );
        __throwing_func( pThis->pSymbols = SymbolTable_Create(NUMBER_OF_SYMBOL_TABLE_HASH_BUCKETS) );
        __throwing_func( pThis->pObjectBuffer = BinaryBuffer_Create(SIZE_OF_OBJECT_AND_DUMMY_BUFFERS) )
        __throwing_func( pThis->pDummyBuffer = BinaryBuffer_Create(SIZE_OF_OBJECT_AND_DUMMY_BUFFERS) );
        pThis->pLineInfo = &pThis->linesHead;
        pThis->pLocalLabelStart = pThis->labelBuffer;
        pThis->maxLocalLabelSize = sizeof(pThis->labelBuffer)-1;
        pThis->pCurrentBuffer = pThis->pObjectBuffer;
    }
    __catch
    {
        __rethrow;
    }
}


__throws Assembler* Assembler_CreateFromFile(const char* pSourceFilename)
{
    Assembler* pThis = NULL;
    
    __try
    {
        __throwing_func( pThis = allocateAndZeroObject() );
        __throwing_func( commonObjectInit(pThis) );
        __throwing_func( pThis->pTextFile = TextFile_CreateFromFile(pSourceFilename) );
        pThis->pSourceFilename = pSourceFilename;
    }
    __catch
    {
        Assembler_Free(pThis);
        __rethrow_and_return(NULL);
    }

    return pThis;
}


static void freeLines(Assembler* pThis);
void Assembler_Free(Assembler* pThis)
{
    if (!pThis)
        return;
    
    freeLines(pThis);
    LineBuffer_Free(pThis->pLineText);
    ListFile_Free(pThis->pListFile);
    BinaryBuffer_Free(pThis->pDummyBuffer);
    BinaryBuffer_Free(pThis->pObjectBuffer);
    SymbolTable_Free(pThis->pSymbols);
    TextFile_Free(pThis->pTextFile);
    free(pThis);
}

static void freeLines(Assembler* pThis)
{
    LineInfo* pCurr = pThis->linesHead.pNext;
    
    while (pCurr)
    {
        LineInfo* pNext = pCurr->pNext;
        free(pCurr);
        pCurr = pNext;
    }
}


static void firstPass(Assembler* pThis);
static void parseLine(Assembler* pThis, char* pLine);
static void prepareLineInfoForThisLine(Assembler* pThis, char* pLine);
static void firstPassAssembleLine(Assembler* pThis);
static void handleOpcode(Assembler* pThis, const OpCodeEntry* pOpcodeEntry);
static void handleImpliedAddressingMode(Assembler* pThis, unsigned char opcodeImplied);
static void logInvalidAddressingModeError(Assembler* pThis);
static void emitSingleByteInstruction(Assembler* pThis, unsigned char opCode);
static unsigned char* allocateObjectBytes(Assembler* pThis, size_t bytesToAllocate);
static unsigned char* reallocObjectBytes(Assembler* pThis, unsigned char* pToRealloc, size_t bytesToAllocate);
static void handleZeroPageOrAbsoluteAddressingMode(Assembler*         pThis, 
                                                   AddressingMode*    pAddressingMode, 
                                                   const OpCodeEntry* pOpcodeEntry);
static void handleRelativeAddressingMode(Assembler* pThis, AddressingMode* pAddressingMode, unsigned char opcodeRelative);
static int expressionContainsForwardReference(Expression* pExpression);
static void handleZeroPageOrAbsoluteAddressingModes(Assembler*         pThis, 
                                                    AddressingMode*    pAddressingMode, 
                                                    unsigned char      opcodeZeroPage,
                                                    unsigned char      opcodeAbsolute);
static void emitTwoByteInstruction(Assembler* pThis, unsigned char opCode, unsigned short value);
static void emitThreeByteInstruction(Assembler* pThis, unsigned char opCode, unsigned short value);
static void handleImmediateAddressingMode(Assembler*      pThis, 
                                          AddressingMode* pAddressingMode, 
                                          unsigned char   opcodeImmediate);
static void handleTwoByteAddressingMode(Assembler*      pThis, 
                                        AddressingMode* pAddressingMode, 
                                        unsigned char   opcode);
static void handleZeroPageOrAbsoluteIndexedIndirectAddressingMode(Assembler*         pThis, 
                                                                  AddressingMode*    pAddressingMode, 
                                                                  const OpCodeEntry* pOpcodeEntry);
static void handleIndirectIndexedAddressingMode(Assembler*      pThis, 
                                                AddressingMode* pAddressingMode, 
                                                unsigned char   opcodeIndirectIndexed);
static void handleZeroPageOrAbsoluteIndexedXAddressingMode(Assembler*         pThis, 
                                                           AddressingMode*    pAddressingMode, 
                                                           const OpCodeEntry* pOpcodeEntry);
static void handleZeroPageOrAbsoluteIndexedYAddressingMode(Assembler*         pThis, 
                                                           AddressingMode*    pAddressingMode, 
                                                           const OpCodeEntry* pOpcodeEntry);
static void handleZeroPageOrAbsoluteIndirectAddressingMode(Assembler*         pThis, 
                                                           AddressingMode*    pAddressingMode, 
                                                           const OpCodeEntry* pOpcodeEntry);
static void handleEQU(Assembler* pThis);
static void validateEQULabelFormat(Assembler* pThis);
static void validateLabelFormat(Assembler* pThis);
static Symbol* attemptToAddSymbol(Assembler* pThis, const char* pSymbolName, Expression* pExpression);
static int isSymbolAlreadyDefined(Symbol* pSymbol, LineInfo* pThisLine);
static void flagSymbolAsDefined(Symbol* pSymbol, LineInfo* pThisLine);
static void updateLinesWhichForwardReferencedThisLabel(Assembler* pThis, Symbol* pSymbol);
static int symbolContainsForwardReferences(Symbol* pSymbol);
static void updateLineWithForwardReference(Assembler* pThis, Symbol* pSymbol, LineInfo* pLineInfo);
static void ignoreOperator(Assembler* pThis);
static void handleInvalidOperator(Assembler* pThis);
static void handleASC(Assembler* pThis);
static const char* fullOperandStringWithSpaces(Assembler* pThis);
static void handleDEND(Assembler* pThis);
static Expression getAbsoluteExpression(Assembler* pThis);
static int isTypeAbsolute(Expression* pExpression);
static void handleDUM(Assembler* pThis);
static int isAlreadyInDUMSection(Assembler* pThis);
static void handleDS(Assembler* pThis);
static void saveDSInfoInLineInfo(Assembler* pThis, unsigned short repeatCount);
static void handleHEX(Assembler* pThis);
static unsigned char getNextHexByte(const char* pStart, const char** ppNext);
static unsigned char hexCharToNibble(char value);
static void logHexParseError(Assembler* pThis);
static unsigned char* reallocObjectBytes(Assembler* pThis, unsigned char* pToRealloc, size_t bytesToAllocate);
static void handleORG(Assembler* pThis);
static void rememberLabel(Assembler* pThis);
static int isLabelToRemember(Assembler* pThis);
static int doesLineContainALabel(Assembler* pThis);
static int wasEQUDirective(Assembler* pThis);
static const char* expandedLabelName(Assembler* pThis, const char* pLabelName, size_t labelLength);
static int isGlobalLabelName(const char* pLabelName);
static int isLocalLabelName(const char* pLabelName);
static char* copySizedLabel(Assembler* pThis, const char* pLabel, size_t labelLength);
static void expandLocalLabelToGloballyUniqueName(Assembler* pThis, const char* pLocalLabelName, size_t length);
static int seenGlobalLabel(Assembler* pThis);
static void rememberGlobalLabel(Assembler* pThis);
static void checkForUndefinedSymbols(Assembler* pThis);
static void checkSymbolForOutstandingForwardReferences(Assembler* pThis, Symbol* pSymbol);
static void secondPass(Assembler* pThis);
void Assembler_Run(Assembler* pThis)
{
    __try
    {
        __throwing_func( firstPass(pThis) );
        checkForUndefinedSymbols(pThis);
        __throwing_func( secondPass(pThis) );
    }
    __catch
    {
        __rethrow;
    }
}

static void firstPass(Assembler* pThis)
{
    char*      pLine = NULL;
    
    while (NULL != (pLine = TextFile_GetNextLine(pThis->pTextFile)))
    {
        __try
            parseLine(pThis, pLine);
        __catch
            __rethrow;
    }
}

static void parseLine(Assembler* pThis, char* pLine)
{
    __try
    {
        __throwing_func( LineBuffer_Set(pThis->pLineText, pLine) );
        __throwing_func( prepareLineInfoForThisLine(pThis, pLine) );
        ParseLine(&pThis->parsedLine, LineBuffer_Get(pThis->pLineText));
        firstPassAssembleLine(pThis);
        rememberLabel(pThis);
        pThis->programCounter += pThis->pLineInfo->machineCodeSize;
    }
    __catch
    {
        __rethrow;
    }
}

static void prepareLineInfoForThisLine(Assembler* pThis, char* pLine)
{
    LineInfo* pLineInfo = malloc(sizeof(*pLineInfo));
    if (!pLineInfo)
        __throw(outOfMemoryException);
        
    memset(pLineInfo, 0, sizeof(*pLineInfo));
    pLineInfo->lineNumber = pThis->pLineInfo->lineNumber + 1;
    pLineInfo->pLineText = pLine;
    pLineInfo->address = pThis->programCounter;
    pThis->pLineInfo->pNext = pLineInfo;
    pThis->pLineInfo = pLineInfo;
}

/* Used for unsupported addressing modes in opcode table entries. */
#define _xXX 0xFF

static void firstPassAssembleLine(Assembler* pThis)
{
    static const OpCodeEntry opcodeTable[] =
    {
        /* Assembler Directives */
        {"=",    handleEQU,  _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"ASC",  handleASC, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"DEND", handleDEND, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"DS",   handleDS,   _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"DUM",  handleDUM,  _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"EQU",  handleEQU,  _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"LST",  ignoreOperator, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"HEX",  handleHEX,  _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"ORG",  handleORG,  _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        
        /* 6502 Instructions */
//        {"ASL", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"ASL", NULL, _xXX, 0x0E, 0x06, 0x0A, _xXX, _xXX, 0x16, _xXX, 0x1E, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"BCS", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0xB0, _xXX, _xXX, _xXX},
        {"BEQ", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0xF0, _xXX, _xXX, _xXX},
        {"BIT", NULL, 0x89, 0x2C, 0x24, _xXX, _xXX, _xXX, 0x34, _xXX, 0x3C, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"BMI", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x30, _xXX, _xXX, _xXX},
        {"BNE", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0xD0, _xXX, _xXX, _xXX},
        {"BPL", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x10, _xXX, _xXX, _xXX},
        {"CLC", NULL, _xXX, _xXX, _xXX, 0x18, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"CMP", NULL, 0xC9, 0xCD, 0xC5, _xXX, 0xC1, 0xD1, 0xD5, _xXX, 0xDD, 0xD9, _xXX, _xXX, _xXX, 0xD2},
        {"DEC", NULL, _xXX, 0xCE, 0xC6, _xXX, _xXX, _xXX, 0xD6, _xXX, 0xDE, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"DEX", NULL, _xXX, _xXX, _xXX, 0xCA, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"INC", NULL, _xXX, 0xEE, 0xE6, _xXX, _xXX, _xXX, 0xF6, _xXX, 0xFE, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"INY", NULL, _xXX, _xXX, _xXX, 0xC8, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"JMP", NULL, _xXX, 0x4C, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x6C, 0x7C, _xXX},
        {"JSR", NULL, _xXX, 0x20, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"LDA", NULL, 0xA9, 0xAD, 0xA5, _xXX, 0xA1, 0xB1, 0xB5, _xXX, 0xBD, 0xB9, _xXX, _xXX, _xXX, 0xB2},
        {"LDX", NULL, 0xA2, 0xAE, 0xA6, _xXX, _xXX, _xXX, _xXX, 0xB6, _xXX, 0xBE, _xXX, _xXX, _xXX, _xXX},
        {"LDY", NULL, 0xA0, 0xAC, 0xA4, _xXX, _xXX, _xXX, 0xB4, _xXX, 0xBC, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"LSR", NULL, _xXX, 0x4E, 0x46, 0x4A, _xXX, _xXX, 0x56, _xXX, 0x5E, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"ORA", NULL, 0x09, 0x0D, 0x05, _xXX, 0x01, 0x11, 0x15, _xXX, 0x1D, 0x19, _xXX, _xXX, _xXX, 0x12},
        {"RTS", NULL, _xXX, _xXX, _xXX, 0x60, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"STA", NULL, _xXX, 0x8D, 0x85, _xXX, 0x81, 0x91, 0x95, _xXX, 0x9D, 0x99, _xXX, _xXX, _xXX, 0x92},
        {"STX", NULL, _xXX, 0x8E, 0x86, _xXX, _xXX, _xXX, _xXX, 0x96, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"STY", NULL, _xXX, 0x8C, 0x84, _xXX, _xXX, _xXX, 0x94, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"TXA", NULL, _xXX, _xXX, _xXX, 0x8A, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    };
    size_t i;
    
    if (pThis->parsedLine.pOperator == NULL)
        return;
    
    for (i = 0 ; i < ARRAYSIZE(opcodeTable) ; i++)
    {
        if (0 == strcasecmp(pThis->parsedLine.pOperator, opcodeTable[i].pOperator))
        {
            handleOpcode(pThis, &opcodeTable[i]);
            return;
        }
    }
    
    handleInvalidOperator(pThis);
}

static void handleOpcode(Assembler* pThis, const OpCodeEntry* pOpcodeEntry)
{
    AddressingMode addressingMode;

    if (pOpcodeEntry->directiveHandler)
    {
        pOpcodeEntry->directiveHandler(pThis);
        return;
    }
    
    __try
        addressingMode = AddressingMode_Eval(pThis, pThis->parsedLine.pOperands);
    __catch
        __nothrow;
        
    switch (addressingMode.mode)
    {
    /* Invalid mode gets caught in try/catch block above but placing here silences compiler warning and keeps 
       100% code coverage. */
    case ADDRESSING_MODE_INVALID:
    case ADDRESSING_MODE_ABSOLUTE:
        handleZeroPageOrAbsoluteAddressingMode(pThis, &addressingMode, pOpcodeEntry);
        break;
    case ADDRESSING_MODE_IMMEDIATE:
        handleImmediateAddressingMode(pThis, &addressingMode, pOpcodeEntry->opcodeImmediate);
        break;
    case ADDRESSING_MODE_IMPLIED:
        handleImpliedAddressingMode(pThis, pOpcodeEntry->opcodeImplied);
        break;
    case ADDRESSING_MODE_INDEXED_INDIRECT:
        handleZeroPageOrAbsoluteIndexedIndirectAddressingMode(pThis, &addressingMode, pOpcodeEntry);
        break;
    case ADDRESSING_MODE_INDIRECT_INDEXED:
        handleIndirectIndexedAddressingMode(pThis, &addressingMode, pOpcodeEntry->opcodeIndirectIndexed);
        break;
    case ADDRESSING_MODE_ABSOLUTE_INDEXED_X:
        handleZeroPageOrAbsoluteIndexedXAddressingMode(pThis, &addressingMode, pOpcodeEntry);
        break;
    case ADDRESSING_MODE_ABSOLUTE_INDEXED_Y:
        handleZeroPageOrAbsoluteIndexedYAddressingMode(pThis, &addressingMode, pOpcodeEntry);
        break;
    case ADDRESSING_MODE_INDIRECT:
        handleZeroPageOrAbsoluteIndirectAddressingMode(pThis, &addressingMode, pOpcodeEntry);
        break;
    }
}

static void handleImpliedAddressingMode(Assembler* pThis, unsigned char opcodeImplied)
{
    if (opcodeImplied == _xXX)
    {
        logInvalidAddressingModeError(pThis);
        return;
    }
    emitSingleByteInstruction(pThis, opcodeImplied);
}

static void logInvalidAddressingModeError(Assembler* pThis)
{
    LOG_ERROR(pThis, "Addressing mode of '%s' is not supported for '%s' instruction.", 
              pThis->parsedLine.pOperands, pThis->parsedLine.pOperator);
}

static void emitSingleByteInstruction(Assembler* pThis, unsigned char opCode)
{
    __try
        pThis->pLineInfo->pMachineCode = allocateObjectBytes(pThis, 1);
    __catch
        __nothrow;
    pThis->pLineInfo->pMachineCode[0] = opCode;
    pThis->pLineInfo->machineCodeSize = 1;
}

static unsigned char* allocateObjectBytes(Assembler* pThis, size_t bytesToAllocate)
{
    return reallocObjectBytes(pThis, NULL, bytesToAllocate);
}

static unsigned char* reallocObjectBytes(Assembler* pThis, unsigned char* pToRealloc, size_t bytesToAllocate)
{
    unsigned char* pAlloc;
    __try
    {
        pAlloc = BinaryBuffer_Realloc(pThis->pCurrentBuffer, pToRealloc, bytesToAllocate);
    }
    __catch
    {
        LOG_ERROR(pThis, "Exceeded the %d allowed bytes in the object file.", SIZE_OF_OBJECT_AND_DUMMY_BUFFERS);
        __rethrow_and_return(NULL);
    }
    
    return pAlloc;
}

static void handleZeroPageOrAbsoluteAddressingMode(Assembler*         pThis, 
                                                   AddressingMode*    pAddressingMode, 
                                                   const OpCodeEntry* pOpcodeEntry)
{
    if (pOpcodeEntry->opcodeRelative != _xXX)
    {
        handleRelativeAddressingMode(pThis, pAddressingMode, pOpcodeEntry->opcodeRelative);
        return;
    }
    
    handleZeroPageOrAbsoluteAddressingModes(pThis, 
                                            pAddressingMode, 
                                            pOpcodeEntry->opcodeZeroPage, pOpcodeEntry->opcodeAbsolute);
}

static void handleZeroPageOrAbsoluteAddressingModes(Assembler*         pThis, 
                                                    AddressingMode*    pAddressingMode, 
                                                    unsigned char      opcodeZeroPage,
                                                    unsigned char      opcodeAbsolute)
{
    if (pAddressingMode->expression.type == TYPE_ZEROPAGE && opcodeZeroPage != _xXX)
        emitTwoByteInstruction(pThis, opcodeZeroPage, pAddressingMode->expression.value);
    else if (opcodeAbsolute != _xXX)
        emitThreeByteInstruction(pThis, opcodeAbsolute, pAddressingMode->expression.value);
    else
        logInvalidAddressingModeError(pThis);
}

static void handleRelativeAddressingMode(Assembler* pThis, AddressingMode* pAddressingMode, unsigned char opcodeRelative)
{
    unsigned short nextInstructionAddress = pThis->pLineInfo->address + 2;
    int            offset = (int)pAddressingMode->expression.value - (int)nextInstructionAddress;
    
    if (!expressionContainsForwardReference(&pAddressingMode->expression) && (offset < -128 || offset > 127))
    {
        LOG_ERROR(pThis, "Relative offset of '%s' exceeds the allowed -128 to 127 range.", pThis->parsedLine.pOperands);
        return;
    }
    
    emitTwoByteInstruction(pThis, opcodeRelative, (unsigned short)offset);
}

static int expressionContainsForwardReference(Expression* pExpression)
{
    return pExpression->flags & EXPRESSION_FLAG_FORWARD_REFERENCE;
}

static void emitTwoByteInstruction(Assembler* pThis, unsigned char opCode, unsigned short value)
{
    __try
        pThis->pLineInfo->pMachineCode = allocateObjectBytes(pThis, 2);
    __catch
        __nothrow;
    pThis->pLineInfo->pMachineCode[0] = opCode;
    pThis->pLineInfo->pMachineCode[1] = LO_BYTE(value);
    pThis->pLineInfo->machineCodeSize = 2;
}

static void emitThreeByteInstruction(Assembler* pThis, unsigned char opCode, unsigned short value)
{
    __try
        pThis->pLineInfo->pMachineCode = allocateObjectBytes(pThis, 3);
    __catch
        __nothrow;
    pThis->pLineInfo->pMachineCode[0] = opCode;
    pThis->pLineInfo->pMachineCode[1] = LO_BYTE(value);
    pThis->pLineInfo->pMachineCode[2] = HI_BYTE(value);
    pThis->pLineInfo->machineCodeSize = 3;
}

static void handleImmediateAddressingMode(Assembler*      pThis, 
                                          AddressingMode* pAddressingMode, 
                                          unsigned char   opcodeImmediate)
{
    handleTwoByteAddressingMode(pThis, pAddressingMode, opcodeImmediate);
}

static void handleTwoByteAddressingMode(Assembler*      pThis, 
                                        AddressingMode* pAddressingMode, 
                                        unsigned char   opcode)
{
    if (opcode == _xXX)
    {
        logInvalidAddressingModeError(pThis);
        return;
    }
    emitTwoByteInstruction(pThis, opcode, pAddressingMode->expression.value);
}

static void handleZeroPageOrAbsoluteIndexedIndirectAddressingMode(Assembler*         pThis, 
                                                                  AddressingMode*    pAddressingMode, 
                                                                  const OpCodeEntry* pOpcodeEntry)
{
    handleZeroPageOrAbsoluteAddressingModes(pThis, 
                                            pAddressingMode, 
                                            pOpcodeEntry->opcodeZeroPageIndexedIndirect, 
                                            pOpcodeEntry->opcodeAbsoluteIndexedIndirect);
}

static void handleIndirectIndexedAddressingMode(Assembler*      pThis, 
                                                AddressingMode* pAddressingMode, 
                                                unsigned char   opcodeIndirectIndexed)
{
    handleTwoByteAddressingMode(pThis, pAddressingMode, opcodeIndirectIndexed);
}

static void handleZeroPageOrAbsoluteIndexedXAddressingMode(Assembler*         pThis, 
                                                           AddressingMode*    pAddressingMode, 
                                                           const OpCodeEntry* pOpcodeEntry)
{
    handleZeroPageOrAbsoluteAddressingModes(pThis, 
                                            pAddressingMode, 
                                            pOpcodeEntry->opcodeZeroPageIndexedX, 
                                            pOpcodeEntry->opcodeAbsoluteIndexedX);
}

static void handleZeroPageOrAbsoluteIndexedYAddressingMode(Assembler*         pThis, 
                                                           AddressingMode*    pAddressingMode, 
                                                           const OpCodeEntry* pOpcodeEntry)
{
    handleZeroPageOrAbsoluteAddressingModes(pThis, 
                                            pAddressingMode, 
                                            pOpcodeEntry->opcodeZeroPageIndexedY, 
                                            pOpcodeEntry->opcodeAbsoluteIndexedY);
}

static void handleZeroPageOrAbsoluteIndirectAddressingMode(Assembler*         pThis, 
                                                           AddressingMode*    pAddressingMode, 
                                                           const OpCodeEntry* pOpcodeEntry)
{
    handleZeroPageOrAbsoluteAddressingModes(pThis, 
                                            pAddressingMode, 
                                            pOpcodeEntry->opcodeZeroPageIndirect, 
                                            pOpcodeEntry->opcodeAbsoluteIndirect);
}

static void handleEQU(Assembler* pThis)
{
    __try
    {
        Symbol*    pSymbol = NULL;
        Expression expression;
        
        pThis->pLineInfo->flags |= LINEINFO_FLAG_WAS_EQU;
        __throwing_func( expression = ExpressionEval(pThis, pThis->parsedLine.pOperands) );
        __throwing_func( validateEQULabelFormat(pThis) );
        __throwing_func( pSymbol = attemptToAddSymbol(pThis, pThis->parsedLine.pLabel, &expression) );
        pThis->pLineInfo->pSymbol = pSymbol;
    }
    __catch
    {
        __nothrow;
    }
}

static void validateEQULabelFormat(Assembler* pThis)
{
    if (pThis->parsedLine.pLabel[0] == ':')
    {
        LOG_ERROR(pThis, "'%s' can't be a local label when used with EQU.", pThis->parsedLine.pLabel);
        __throw(invalidArgumentException);
    }
    
    validateLabelFormat(pThis);
}

static void validateLabelFormat(Assembler* pThis)
{
    const char* pCurr;
    
    if (pThis->parsedLine.pLabel[0] < ':')
    {
        LOG_ERROR(pThis, "'%s' label starts with invalid character.", pThis->parsedLine.pLabel);
        __throw(invalidArgumentException);
    }
    
    pCurr = &pThis->parsedLine.pLabel[1];
    while (*pCurr)
    {
        if (*pCurr < '0')
        {
            LOG_ERROR(pThis, "'%s' label contains invalid character, '%c'.", pThis->parsedLine.pLabel, *pCurr);
            __throw(invalidArgumentException);
        }
        pCurr++;
    }
}

static Symbol* attemptToAddSymbol(Assembler* pThis, const char* pSymbolName, Expression* pExpression)
{
    Symbol* pSymbol = SymbolTable_Find(pThis->pSymbols, pSymbolName);
    if (pSymbol && isSymbolAlreadyDefined(pSymbol, pThis->pLineInfo))
    {
        LOG_ERROR(pThis, "'%s' symbol has already been defined.", pSymbolName);
        __throw_and_return(invalidArgumentException, NULL);
    }
    if (!pSymbol)
    {
        __try
        {
            pSymbol = SymbolTable_Add(pThis->pSymbols, pSymbolName);
        }
        __catch
        {
            LOG_ERROR(pThis, "Failed to allocate space for '%s' symbol.", pSymbolName);
            __rethrow_and_return(NULL);
        }
    }
    flagSymbolAsDefined(pSymbol, pThis->pLineInfo);
    pSymbol->expression = *pExpression;

    __try
    {
        updateLinesWhichForwardReferencedThisLabel(pThis, pSymbol);
    }
    __catch
    {
        LOG_ERROR(pThis, "Failed to allocate space for updating forward references to '%s' symbol.", pSymbolName);
        __rethrow_and_return(NULL);
    }

    return pSymbol;
}

static int isSymbolAlreadyDefined(Symbol* pSymbol, LineInfo* pThisLine)
{
    return pSymbol->pDefinedLine != NULL && pSymbol->pDefinedLine != pThisLine;
}

static void flagSymbolAsDefined(Symbol* pSymbol, LineInfo* pThisLine)
{
    pSymbol->pDefinedLine = pThisLine;
}

static void updateLinesWhichForwardReferencedThisLabel(Assembler* pThis, Symbol* pSymbol)
{
    LineInfo* pCurr = NULL;
    
    if (symbolContainsForwardReferences(pSymbol))
        return;
        
    Symbol_LineReferenceEnumStart(pSymbol);
    while (NULL != (pCurr = Symbol_LineReferenceEnumNext(pSymbol)))
    {
        __try
            updateLineWithForwardReference(pThis, pSymbol, pCurr);
        __catch
            __rethrow;
    }
}

static int symbolContainsForwardReferences(Symbol* pSymbol)
{
    return pSymbol->expression.flags & EXPRESSION_FLAG_FORWARD_REFERENCE;
}

static void updateLineWithForwardReference(Assembler* pThis, Symbol* pSymbol, LineInfo* pLineInfo)
{
    LineBuffer* pLineBuffer = NULL;
    size_t      machineCodeSizeOrig;
    LineInfo*   pLineInfoSave;
    ParsedLine  parsedLineSave;
    
    machineCodeSizeOrig = pLineInfo->machineCodeSize;
    pLineInfoSave = pThis->pLineInfo;
    parsedLineSave = pThis->parsedLine;
    pThis->pLineInfo = pLineInfo;

    __try
    {
        Symbol_LineReferenceRemove(pSymbol, pLineInfo);
        __throwing_func( pLineBuffer = LineBuffer_Create() );
        __throwing_func( LineBuffer_Set(pLineBuffer, pLineInfo->pLineText) );
        ParseLine(&pThis->parsedLine, LineBuffer_Get(pLineBuffer));
        firstPassAssembleLine(pThis);
    }
    __catch
    {
        /* Fall through to cleanup code below. */
    }

    if (machineCodeSizeOrig != pLineInfo->machineCodeSize)
        LOG_ERROR(pThis, "Couldn't properly infer size of '%s' forward reference.", pSymbol->pKey);
    
    pThis->parsedLine = parsedLineSave;
    LineBuffer_Free(pLineBuffer);
    pThis->pLineInfo = pLineInfoSave;
}

static void ignoreOperator(Assembler* pThis)
{
}

static void handleInvalidOperator(Assembler* pThis)
{
    LOG_ERROR(pThis, "'%s' is not a recognized mnemonic or macro.", pThis->parsedLine.pOperator);
}

static void handleASC(Assembler* pThis)
{
    const char*    pOperands = fullOperandStringWithSpaces(pThis);
    char           delimiter = *pOperands;
    unsigned char* pAlloc = NULL;
    const char*    pCurr = &pOperands[1];
    size_t         i = 0;
    unsigned char  mask = delimiter < '\'' ? 0x80 : 0x00;

    while (*pCurr && *pCurr != delimiter)
    {
        __try
        {
            unsigned char   byte = *pCurr | mask;
            __throwing_func( pAlloc = reallocObjectBytes(pThis, pAlloc, i+1) );
            pThis->pLineInfo->pMachineCode = pAlloc;
            pThis->pLineInfo->pMachineCode[i++] = byte;
            pCurr++;
        }
        __catch
        {
            __nothrow;
        }
    }
    
    if (*pCurr == '\0')
        LOG_ERROR(pThis, "%s didn't end with the expected %c delimiter.", pThis->parsedLine.pOperands, delimiter);
    pThis->pLineInfo->machineCodeSize = i;
}

static const char* fullOperandStringWithSpaces(Assembler* pThis)
{
    int offsetOfOperandsWithInLine = pThis->parsedLine.pOperands - LineBuffer_Get(pThis->pLineText);
    return pThis->pLineInfo->pLineText + offsetOfOperandsWithInLine;
}

static void handleDEND(Assembler* pThis)
{
    if (!isAlreadyInDUMSection(pThis))
    {
        LOG_ERROR(pThis, "%s isn't allowed without a preceding DUM directive.", pThis->parsedLine.pOperator);
        return;
    }

    pThis->programCounter = pThis->programCounterBeforeDUM;
    pThis->pCurrentBuffer = pThis->pObjectBuffer;
}

static void handleDUM(Assembler* pThis)
{
    Expression expression;
    
    __try
        expression = getAbsoluteExpression(pThis);
    __catch
        __nothrow;

    if (!isAlreadyInDUMSection(pThis))
        pThis->programCounterBeforeDUM = pThis->programCounter;
    pThis->programCounter = expression.value;
    pThis->pCurrentBuffer = pThis->pDummyBuffer;
}

static Expression getAbsoluteExpression(Assembler* pThis)
{
    Expression expression;
    
    __try
    {
        __throwing_func( expression = ExpressionEval(pThis, pThis->parsedLine.pOperands) );
        if (!isTypeAbsolute(&expression))
        {
            LOG_ERROR(pThis, "'%s' doesn't specify an absolute address.", pThis->parsedLine.pOperands);
            __throw_and_return(invalidArgumentException, expression);
        }
    }
    __catch
    {
        __rethrow_and_return(expression);
    }
    return expression;
}

static int isTypeAbsolute(Expression* pExpression)
{
    return pExpression->type == TYPE_ZEROPAGE ||
           pExpression->type == TYPE_ABSOLUTE;
}

static int isAlreadyInDUMSection(Assembler* pThis)
{
    return pThis->pCurrentBuffer == pThis->pDummyBuffer;
}

static void handleDS(Assembler* pThis)
{
    Expression expression;
    
    __try
        expression = getAbsoluteExpression(pThis);
    __catch
        __nothrow;

    saveDSInfoInLineInfo(pThis, expression.value);
}

static void saveDSInfoInLineInfo(Assembler* pThis, unsigned short repeatCount)
{
    __try
        pThis->pLineInfo->pMachineCode = allocateObjectBytes(pThis, repeatCount);
    __catch
        __nothrow;

    pThis->pLineInfo->machineCodeSize = repeatCount;
    memset(pThis->pLineInfo->pMachineCode, 0, repeatCount);
}

static void handleHEX(Assembler* pThis)
{
    unsigned char* pAlloc = NULL;
    const char*    pCurr = pThis->parsedLine.pOperands;
    size_t         i = 0;

    while (*pCurr && i < 32)
    {
        __try
        {
            unsigned int   byte;
            const char*    pNext;

            __throwing_func( byte = getNextHexByte(pCurr, &pNext) );
            __throwing_func( pAlloc = reallocObjectBytes(pThis, pAlloc, i+1) );
            
            pThis->pLineInfo->pMachineCode = pAlloc;
            pThis->pLineInfo->pMachineCode[i++] = byte;
            pCurr = pNext;
        }
        __catch
        {
            logHexParseError(pThis);
            __nothrow;
        }
    }
    
    if (*pCurr)
    {
        LOG_ERROR(pThis, "'%s' contains more than 32 values.", pThis->parsedLine.pOperands);
        return;
    }
    pThis->pLineInfo->machineCodeSize = i;
}

static unsigned char getNextHexByte(const char* pStart, const char** ppNext)
{
    const char*   pCurr = pStart;
    unsigned char value;
    
    __try
    {
        __throwing_func( value = hexCharToNibble(*pCurr++) << 4 );
        if (*pCurr == '\0')
            __throw_and_return(invalidArgumentException, 0);
        __throwing_func( value |= hexCharToNibble(*pCurr++) );

        if (*pCurr == ',')
            pCurr++;
    }
    __catch
    {
        __rethrow_and_return(0);
    }

    *ppNext = pCurr;
    return value;
}

static unsigned char hexCharToNibble(char value)
{
    if (value >= '0' && value <= '9')
        return value - '0';
    if (value >= 'a' && value <= 'f')
        return value - 'a' + 10;
    if (value >= 'A' && value <= 'F')
        return value - 'a' + 10;
    __throw_and_return(invalidHexDigitException, 0);
}

static void logHexParseError(Assembler* pThis)
{
    if (getExceptionCode() == invalidArgumentException)
        LOG_ERROR(pThis, "'%s' doesn't contain an even number of hex digits.", pThis->parsedLine.pOperands);
    else if (getExceptionCode() == invalidHexDigitException)
        LOG_ERROR(pThis, "'%s' contains an invalid hex digit.", pThis->parsedLine.pOperands);
}

static void handleORG(Assembler* pThis)
{
    Expression expression;
    
    __try
    {
        __throwing_func( expression = getAbsoluteExpression(pThis) );
        pThis->programCounter = expression.value;
    }
    __catch
    {
        __nothrow;
    }
}

static void rememberLabel(Assembler* pThis)
{
    if (!isLabelToRemember(pThis))
        return;

    __try
    {
        const char* pExpandedLabelName = NULL;
        Symbol*     pSymbol = NULL;
        Expression  expression;
    
        __throwing_func( validateLabelFormat(pThis) );
        __throwing_func( pExpandedLabelName = expandedLabelName(pThis, pThis->parsedLine.pLabel, strlen(pThis->parsedLine.pLabel)) );
        expression = ExpressionEval_CreateAbsoluteExpression(pThis->programCounter);
        __throwing_func( pSymbol = attemptToAddSymbol(pThis, pExpandedLabelName, &expression) );
        __throwing_func( rememberGlobalLabel(pThis) );
    }
    __catch
    {
        __nothrow;
    }
}

static int isLabelToRemember(Assembler* pThis)
{
    return doesLineContainALabel(pThis) && !wasEQUDirective(pThis);
}

static int doesLineContainALabel(Assembler* pThis)
{
    return pThis->parsedLine.pLabel != NULL;
}

static int wasEQUDirective(Assembler* pThis)
{
    return pThis->pLineInfo->flags & LINEINFO_FLAG_WAS_EQU;
}

static const char* expandedLabelName(Assembler* pThis, const char* pLabelName, size_t labelLength)
{
    /* UNDONE: I don't like taking the copy of the global label here as it is really just a hack.  Might be better
               to pass around a SizedString between these routines...all the way into the SymbolTable routines. Sized 
               string could even be passed around between other routines to decrease the number of memory allocations
               made to take copies of const char* pointers. */
    if (isGlobalLabelName(pLabelName))
        return copySizedLabel(pThis, pLabelName, labelLength);
    
    expandLocalLabelToGloballyUniqueName(pThis, pLabelName, labelLength);    
    return pThis->labelBuffer;
}

static int isGlobalLabelName(const char* pLabelName)
{
    return !isLocalLabelName(pLabelName);
}

static int isLocalLabelName(const char* pLabelName)
{
    return *pLabelName == ':';
}

static char* copySizedLabel(Assembler* pThis, const char* pLabel, size_t labelLength)
{
    if (labelLength > pThis->maxLocalLabelSize)
    {
        LOG_ERROR(pThis, "'%.*s' label is too long.", labelLength, pLabel);
        __throw_and_return(bufferOverrunException, NULL);
    }
    memcpy(pThis->pLocalLabelStart, pLabel, labelLength);
    pThis->pLocalLabelStart[labelLength] = '\0';
    
    return pThis->pLocalLabelStart;
}

static void expandLocalLabelToGloballyUniqueName(Assembler* pThis, const char* pLocalLabelName, size_t length)
{
    if (!seenGlobalLabel(pThis))
    {
        LOG_ERROR(pThis, "'%.*s' local label isn't allowed before first global label.", length, pLocalLabelName);
        __throw(invalidArgumentException);
    }
    copySizedLabel(pThis, pLocalLabelName, length);
}

static int seenGlobalLabel(Assembler* pThis)
{
    return pThis->labelBuffer[0] != '\0';
}

static void rememberGlobalLabel(Assembler* pThis)
{
    size_t length = strlen(pThis->parsedLine.pLabel);

    if (!isGlobalLabelName(pThis->parsedLine.pLabel))
        return;

    /* The length of the global label was already validated earlier in the expandedLabelName()'s call to 
       copySizedLabel. */
    assert ( length < sizeof(pThis->labelBuffer) );
    memcpy(pThis->labelBuffer, pThis->parsedLine.pLabel, length);
    pThis->pLocalLabelStart = &pThis->labelBuffer[length];
    pThis->maxLocalLabelSize = sizeof(pThis->labelBuffer) - length - 1;
}

static void checkForUndefinedSymbols(Assembler* pThis)
{
    Symbol* pSymbol;
    
    SymbolTable_EnumStart(pThis->pSymbols);
    while (NULL != (pSymbol = SymbolTable_EnumNext(pThis->pSymbols)))
        checkSymbolForOutstandingForwardReferences(pThis, pSymbol);
}

static void checkSymbolForOutstandingForwardReferences(Assembler* pThis, Symbol* pSymbol)
{
    LineInfo* pLineInfo;
    
    if (!pSymbol->pLineReferences)
        return;
        
    Symbol_LineReferenceEnumStart(pSymbol);
    while (NULL != (pLineInfo = Symbol_LineReferenceEnumNext(pSymbol)))
    {
        pThis->pLineInfo = pLineInfo;
        LOG_ERROR(pThis, "The '%s' label is undefined.", pSymbol->pKey);
    }
}

static void secondPass(Assembler* pThis)
{
    LineInfo* pCurr = pThis->linesHead.pNext;
    
    while(pCurr)
    {
        ListFile_OutputLine(pThis->pListFile, pCurr);
        pCurr = pCurr->pNext;
    }
}


unsigned int Assembler_GetErrorCount(Assembler* pThis)
{
    return pThis->errorCount;
}


__throws Symbol* Assembler_FindLabel(Assembler* pThis, const char* pLabelName, size_t labelLength)
{
    Symbol*     pSymbol = NULL;
    const char* pExpandedName = NULL;
    
    __try
    {
        __throwing_func( pExpandedName = expandedLabelName(pThis, pLabelName, labelLength) );
        pSymbol = SymbolTable_Find(pThis->pSymbols, pExpandedName);
        if (!pSymbol)
            __throwing_func( pSymbol = SymbolTable_Add(pThis->pSymbols, pExpandedName) );
        if (!isSymbolAlreadyDefined(pSymbol, NULL))
            __throwing_func( Symbol_LineReferenceAdd(pSymbol, pThis->pLineInfo) );
    }
    __catch
    {
        __rethrow_and_return(NULL);
    }

    return pSymbol;
}
