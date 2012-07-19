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
        pThis->pLineInfo = &pThis->linesHead;
        pThis->pLocalLabelStart = pThis->labelBuffer;
        pThis->maxLocalLabelSize = sizeof(pThis->labelBuffer)-1;
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
static void handleZeroPageOrAbsoluteAddressingMode(Assembler*         pThis, 
                                                   AddressingMode*    pAddressingMode, 
                                                   const OpCodeEntry* pOpcodeEntry);
static void emitTwoByteInstruction(Assembler* pThis, unsigned char opCode, unsigned short value);
static void emitThreeByteInstruction(Assembler* pThis, unsigned char opCode, unsigned short value);
static void handleImmediateAddressingMode(Assembler*      pThis, 
                                          AddressingMode* pAddressingMode, 
                                          unsigned char   opcodeImmediate);
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
static void handleHEX(Assembler* pThis);
static unsigned char getNextHexByte(const char* pStart, const char** ppNext);
static unsigned char hexCharToNibble(char value);
static void logHexParseError(Assembler* pThis);
static void handleORG(Assembler* pThis);
static int isTypeAbsolute(Expression* pExpression);
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
        {"=",   handleEQU, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"EQU", handleEQU, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"LST", ignoreOperator, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"HEX", handleHEX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"ORG", handleORG, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        
        /* 6502 Instructions */
        {"CMP", NULL, 0xC9, 0xCD, 0xC5, _xXX, 0xC1, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"JSR", NULL, _xXX, 0x20, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"LDA", NULL, 0xA9, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"LDX", NULL, _xXX, 0xAE, 0xA6, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"LDY", NULL, _xXX, 0xAC, 0xA4, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"LSR", NULL, _xXX, _xXX, _xXX, 0x4A, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"ORA", NULL, 0x09, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
        {"STA", NULL, _xXX, 0x8D, 0x85, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
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
    case ADDRESSING_MODE_ABSOLUTE:
        handleZeroPageOrAbsoluteAddressingMode(pThis, &addressingMode, pOpcodeEntry);
        break;
    case ADDRESSING_MODE_IMMEDIATE:
        handleImmediateAddressingMode(pThis, &addressingMode, pOpcodeEntry->opcodeImmediate);
        break;
    case ADDRESSING_MODE_IMPLIED:
        handleImpliedAddressingMode(pThis, pOpcodeEntry->opcodeImplied);
        break;
    default:
        logInvalidAddressingModeError(pThis);
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
    pThis->pLineInfo->machineCode[0] = opCode;
    pThis->pLineInfo->machineCodeSize = 1;
}

static void handleZeroPageOrAbsoluteAddressingMode(Assembler*         pThis, 
                                                   AddressingMode*    pAddressingMode, 
                                                   const OpCodeEntry* pOpcodeEntry)
{
    if (pAddressingMode->expression.type == TYPE_ZEROPAGE && pOpcodeEntry->opcodeZeroPage != _xXX)
        emitTwoByteInstruction(pThis, pOpcodeEntry->opcodeZeroPage, pAddressingMode->expression.value);
    else if (pOpcodeEntry->opcodeAbsolute != _xXX)
        emitThreeByteInstruction(pThis, pOpcodeEntry->opcodeAbsolute, pAddressingMode->expression.value);
    else
        logInvalidAddressingModeError(pThis);
}

static void emitTwoByteInstruction(Assembler* pThis, unsigned char opCode, unsigned short value)
{
    pThis->pLineInfo->machineCode[0] = opCode;
    pThis->pLineInfo->machineCode[1] = LO_BYTE(value);
    pThis->pLineInfo->machineCodeSize = 2;
}

static void emitThreeByteInstruction(Assembler* pThis, unsigned char opCode, unsigned short value)
{
    pThis->pLineInfo->machineCode[0] = opCode;
    pThis->pLineInfo->machineCode[1] = LO_BYTE(value);
    pThis->pLineInfo->machineCode[2] = HI_BYTE(value);
    pThis->pLineInfo->machineCodeSize = 3;
}

static void handleImmediateAddressingMode(Assembler*      pThis, 
                                          AddressingMode* pAddressingMode, 
                                          unsigned char   opcodeImmediate)
{
    if (opcodeImmediate == _xXX)
    {
        logInvalidAddressingModeError(pThis);
        return;
    }
    emitTwoByteInstruction(pThis, opcodeImmediate, pAddressingMode->expression.value);
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
    
    pThis->parsedLine = parsedLineSave;
    LineBuffer_Free(pLineBuffer);
    pThis->pLineInfo = pLineInfoSave;

    if (machineCodeSizeOrig != pLineInfo->machineCodeSize)
        LOG_ERROR(pThis, "Couldn't properly infer size of '%s' forward reference.", pSymbol->pKey);
}

static void ignoreOperator(Assembler* pThis)
{
}

static void handleInvalidOperator(Assembler* pThis)
{
    LOG_ERROR(pThis, "'%s' is not a recognized mnemonic or macro.", pThis->parsedLine.pOperator);
}

static void handleHEX(Assembler* pThis)
{
    const char* pCurr = pThis->parsedLine.pOperands;
    size_t      i = 0;

    while (*pCurr && i < sizeof(pThis->pLineInfo->machineCode))
    {
        __try
        {
            unsigned int byte;
            const char*  pNext;

            __throwing_func( byte = getNextHexByte(pCurr, &pNext) );
            pThis->pLineInfo->machineCode[i++] = byte;
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
        __throwing_func( expression = ExpressionEval(pThis, pThis->parsedLine.pOperands) );
        if (!isTypeAbsolute(&expression))
        {
            LOG_ERROR(pThis, "'%s' doesn't specify an absolute address.", pThis->parsedLine.pOperands);
            return;
        }
        pThis->programCounter = expression.value;
    }
    __catch
    {
        __nothrow;
    }
}

static int isTypeAbsolute(Expression* pExpression)
{
    return pExpression->type == TYPE_ZEROPAGE ||
           pExpression->type == TYPE_ABSOLUTE;
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
