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
#include <stdlib.h>
#include "AssemblerPriv.h"
#include "ExpressionEval.h"
#include "AddressingMode.h"
#include "InstructionSets.h"


static void commonObjectInit(Assembler* pThis);
static void createFullInstructionSetTables(Assembler* pThis);
static int compareInstructionSetEntries(const void* pv1, const void* pv2);
static void setOrgInAssemblerAndBinaryBufferModules(Assembler* pThis, unsigned short orgAddress);
__throws Assembler* Assembler_CreateFromString(char* pText)
{
    Assembler* pThis = NULL;
    
    __try
    {
        __throwing_func( pThis = allocateAndZero(sizeof(*pThis)) );
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
        __throwing_func( createFullInstructionSetTables(pThis) );
        pThis->pLineInfo = &pThis->linesHead;
        pThis->pLocalLabelStart = pThis->labelBuffer;
        pThis->maxLocalLabelSize = sizeof(pThis->labelBuffer)-1;
        pThis->pCurrentBuffer = pThis->pObjectBuffer;
        setOrgInAssemblerAndBinaryBufferModules(pThis, 0x8000);
    }
    __catch
    {
        __rethrow;
    }
}

static void createFullInstructionSetTables(Assembler* pThis)
{
    __try
    {
        __throwing_func( pThis->instructionSets[0] = allocateAndZero(sizeof(g_6502InstructionSet)) );
        memcpy(pThis->instructionSets[0], g_6502InstructionSet, sizeof(g_6502InstructionSet));
        pThis->instructionSetSizes[0] = ARRAYSIZE(g_6502InstructionSet);
        qsort(pThis->instructionSets[0], ARRAYSIZE(g_6502InstructionSet), sizeof(g_6502InstructionSet[0]), 
              compareInstructionSetEntries);
        __throwing_func( pThis->instructionSets[1] = allocateAndZero(sizeof(g_6502InstructionSet)) );
        pThis->instructionSetSizes[1] = ARRAYSIZE(g_6502InstructionSet);
        memcpy(pThis->instructionSets[1], pThis->instructionSets[0], sizeof(g_6502InstructionSet));
        __throwing_func( pThis->instructionSets[2] = allocateAndZero(sizeof(g_6502InstructionSet)) );
        pThis->instructionSetSizes[2] = ARRAYSIZE(g_6502InstructionSet);
        memcpy(pThis->instructionSets[2], pThis->instructionSets[0], sizeof(g_6502InstructionSet));
    }
    __catch
    {
        __rethrow;
    }
}

static int compareInstructionSetEntries(const void* pv1, const void* pv2)
{
    OpCodeEntry* p1 = (OpCodeEntry*)pv1;
    OpCodeEntry* p2 = (OpCodeEntry*)pv2;
    
    return strcasecmp(p1->pOperator, p2->pOperator);
}

static void setOrgInAssemblerAndBinaryBufferModules(Assembler* pThis, unsigned short orgAddress)
{
    pThis->programCounter = orgAddress;
    BinaryBuffer_SetOrigin(pThis->pCurrentBuffer, orgAddress);
}


__throws Assembler* Assembler_CreateFromFile(const char* pSourceFilename)
{
    Assembler* pThis = NULL;
    
    __try
    {
        __throwing_func( pThis = allocateAndZero(sizeof(*pThis)) );
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
static void freeInstructionSets(Assembler* pThis);
void Assembler_Free(Assembler* pThis)
{
    if (!pThis)
        return;
    
    freeLines(pThis);
    freeInstructionSets(pThis);
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

static void freeInstructionSets(Assembler* pThis)
{
    size_t i;
    
    for (i = 0 ; i < ARRAYSIZE(pThis->instructionSets) ; i++)
    {
        free(pThis->instructionSets[i]);
    }
}

static void firstPass(Assembler* pThis);
static void parseLine(Assembler* pThis, char* pLine);
static void prepareLineInfoForThisLine(Assembler* pThis, char* pLine);
static void rememberLabel(Assembler* pThis);
static int doesLineContainALabel(Assembler* pThis);
static void validateLabelFormat(Assembler* pThis);
static const char* expandedLabelName(Assembler* pThis, const char* pLabelName, size_t labelLength);
static int isGlobalLabelName(const char* pLabelName);
static int isLocalLabelName(const char* pLabelName);
static char* copySizedLabel(Assembler* pThis, const char* pLabel, size_t labelLength);
static void expandLocalLabelToGloballyUniqueName(Assembler* pThis, const char* pLocalLabelName, size_t length);
static int seenGlobalLabel(Assembler* pThis);
static Symbol* attemptToAddSymbol(Assembler* pThis, const char* pSymbolName, Expression* pExpression);
static int isSymbolAlreadyDefined(Symbol* pSymbol, LineInfo* pThisLine);
static int isVariableLabelName(const char* pSymbolName);
static void flagSymbolAsDefined(Symbol* pSymbol, LineInfo* pThisLine);
static void rememberGlobalLabel(Assembler* pThis);
static void firstPassAssembleLine(Assembler* pThis);
static int compareInstructionSetEntryToOperatorString(const void* pvKey, const void* pvEntry);
static void handleOpcode(Assembler* pThis, const OpCodeEntry* pOpcodeEntry);
static void handleImpliedAddressingMode(Assembler* pThis, unsigned char opcodeImplied);
static void logInvalidAddressingModeError(Assembler* pThis);
static void emitSingleByteInstruction(Assembler* pThis, unsigned char opCode);
static void allocateLineInfoMachineCodeBytes(Assembler* pThis, size_t bytesToAllocate);
static int isMachineCodeAlreadyAllocatedFromForwardReference(Assembler* pThis);
static void verifyThatMachineCodeSizeFromForwardReferenceMatches(Assembler* pThis, size_t bytesToAllocate);
static void reallocLineInfoMachineCodeBytes(Assembler* pThis, size_t bytesToAllocate);
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
static void validateEQULabelFormat(Assembler* pThis);
static void updateLinesWhichForwardReferencedThisLabel(Assembler* pThis, Symbol* pSymbol);
static int symbolContainsForwardReferences(Symbol* pSymbol);
static void updateLineWithForwardReference(Assembler* pThis, Symbol* pSymbol, LineInfo* pLineInfo);
static void handleInvalidOperator(Assembler* pThis);
static const char* fullOperandStringWithSpaces(Assembler* pThis);
static Expression getAbsoluteExpression(Assembler* pThis, SizedString* pOperands);
static int isTypeAbsolute(Expression* pExpression);
static int isAlreadyInDUMSection(Assembler* pThis);
static Expression getCountExpression(Assembler* pThis, SizedString* pString);
static Expression getBytesLeftInPage(Assembler* pThis);
static void saveDSInfoInLineInfo(Assembler* pThis, unsigned short repeatCount, unsigned char fillValue);
static unsigned char getNextHexByte(const char* pStart, const char** ppNext);
static unsigned char hexCharToNibble(char value);
static void logHexParseError(Assembler* pThis);
static void checkForUndefinedSymbols(Assembler* pThis);
static void checkSymbolForOutstandingForwardReferences(Assembler* pThis, Symbol* pSymbol);
static void secondPass(Assembler* pThis);
static void outputListFile(Assembler* pThis);
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
        rememberLabel(pThis);
        firstPassAssembleLine(pThis);
        pThis->programCounter += pThis->pLineInfo->machineCodeSize;
        updateLinesWhichForwardReferencedThisLabel(pThis, pThis->pLineInfo->pSymbol);
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
    pLineInfo->instructionSet = pThis->instructionSet;
    pThis->pLineInfo->pNext = pLineInfo;
    pThis->pLineInfo = pLineInfo;
}

static void rememberLabel(Assembler* pThis)
{
    if (!doesLineContainALabel(pThis))
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

static int doesLineContainALabel(Assembler* pThis)
{
    return pThis->parsedLine.pLabel != NULL;
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

static const char* expandedLabelName(Assembler* pThis, const char* pLabelName, size_t labelLength)
{
    /* UNDONE: I don't like taking the copy of the global label here as it is really just a hack.  Might be better
               to pass around a SizedString between these routines...all the way into the SymbolTable routines. Sized 
               string could even be passed around between other routines to decrease the number of memory allocations
               made to take copies of const char* pointers. */
    if (!isLocalLabelName(pLabelName))
        return copySizedLabel(pThis, pLabelName, labelLength);
    
    expandLocalLabelToGloballyUniqueName(pThis, pLabelName, labelLength);    
    return pThis->labelBuffer;
}

static int isGlobalLabelName(const char* pLabelName)
{
    return !isLocalLabelName(pLabelName) && !isVariableLabelName(pLabelName);
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

static Symbol* attemptToAddSymbol(Assembler* pThis, const char* pSymbolName, Expression* pExpression)
{
    Symbol* pSymbol = SymbolTable_Find(pThis->pSymbols, pSymbolName);
    if (pSymbol && (isSymbolAlreadyDefined(pSymbol, pThis->pLineInfo) && !isVariableLabelName(pSymbolName)))
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

    return pSymbol;
}

static int isSymbolAlreadyDefined(Symbol* pSymbol, LineInfo* pThisLine)
{
    return pSymbol->pDefinedLine != NULL && pSymbol->pDefinedLine != pThisLine;
}

static int isVariableLabelName(const char* pSymbolName)
{
    return pSymbolName[0] == ']';
}

static void flagSymbolAsDefined(Symbol* pSymbol, LineInfo* pThisLine)
{
    pSymbol->pDefinedLine = pThisLine;
    pThisLine->pSymbol = pSymbol;
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

static void firstPassAssembleLine(Assembler* pThis)
{
    const OpCodeEntry* pInstructionSet = pThis->instructionSets[pThis->pLineInfo->instructionSet];
    size_t             instructionSetSize = pThis->instructionSetSizes[pThis->pLineInfo->instructionSet];
    const OpCodeEntry* pFoundEntry;
    
    if (pThis->parsedLine.pOperator == NULL)
        return;
    
    pFoundEntry = bsearch(pThis->parsedLine.pOperator, 
                          pInstructionSet, instructionSetSize, sizeof(*pInstructionSet), 
                          compareInstructionSetEntryToOperatorString);
    if (pFoundEntry)
        handleOpcode(pThis, pFoundEntry);
    else
        handleInvalidOperator(pThis);
}

static int compareInstructionSetEntryToOperatorString(const void* pvKey, const void* pvEntry)
{
    const char* pKey = (const char*)pvKey;
    const OpCodeEntry* pEntry = (OpCodeEntry*)pvEntry;
    
    return strcasecmp(pKey, pEntry->pOperator);
}

static void handleOpcode(Assembler* pThis, const OpCodeEntry* pOpcodeEntry)
{
    AddressingMode addressingMode;

    if (pOpcodeEntry->directiveHandler)
    {
        pOpcodeEntry->directiveHandler(pThis);
        return;
    }
    
    if (pThis->pLineInfo->instructionSet == INSTRUCTION_SET_65816)
    {
        /* UNDONE: snap doesn't currently support this extended instruction set so just emit RTS for all instructions. */
        emitSingleByteInstruction(pThis, 0x60);
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
        allocateLineInfoMachineCodeBytes(pThis, 1);
    __catch
        __nothrow;
    pThis->pLineInfo->pMachineCode[0] = opCode;
}

static void allocateLineInfoMachineCodeBytes(Assembler* pThis, size_t bytesToAllocate)
{
    if (isMachineCodeAlreadyAllocatedFromForwardReference(pThis))
        verifyThatMachineCodeSizeFromForwardReferenceMatches(pThis, bytesToAllocate);
    else
        reallocLineInfoMachineCodeBytes(pThis, bytesToAllocate);
}

static int isMachineCodeAlreadyAllocatedFromForwardReference(Assembler* pThis)
{
    return pThis->pLineInfo->pMachineCode != NULL;
}

static void verifyThatMachineCodeSizeFromForwardReferenceMatches(Assembler* pThis, size_t bytesToAllocate)
{
    if (pThis->pLineInfo->machineCodeSize != bytesToAllocate)
    {
        LOG_ERROR(pThis, "Couldn't properly infer size of a forward reference in '%s' operand.", pThis->parsedLine.pOperands);
        __throw(invalidArgumentException);
    }
}

static void reallocLineInfoMachineCodeBytes(Assembler* pThis, size_t bytesToAllocate)
{
    __try
    {
        __throwing_func( pThis->pLineInfo->pMachineCode = BinaryBuffer_Realloc(pThis->pCurrentBuffer, 
                                                                               pThis->pLineInfo->pMachineCode, 
                                                                               bytesToAllocate) );
        pThis->pLineInfo->machineCodeSize = bytesToAllocate;
                                                                               
    }
    __catch
    {
        LOG_ERROR(pThis, "Exceeded the %d allowed bytes in the object file.", SIZE_OF_OBJECT_AND_DUMMY_BUFFERS);
        pThis->pLineInfo->machineCodeSize = 0;
        __rethrow;
    }
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
        allocateLineInfoMachineCodeBytes(pThis, 2);
    __catch
        __nothrow;
    pThis->pLineInfo->pMachineCode[0] = opCode;
    pThis->pLineInfo->pMachineCode[1] = LO_BYTE(value);
}

static void emitThreeByteInstruction(Assembler* pThis, unsigned char opCode, unsigned short value)
{
    __try
        allocateLineInfoMachineCodeBytes(pThis, 3);
    __catch
        __nothrow;
    pThis->pLineInfo->pMachineCode[0] = opCode;
    pThis->pLineInfo->pMachineCode[1] = LO_BYTE(value);
    pThis->pLineInfo->pMachineCode[2] = HI_BYTE(value);
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
        Symbol*    pSymbol = pThis->pLineInfo->pSymbol;
        Expression expression;
        
        __throwing_func( expression = ExpressionEval(pThis, pThis->parsedLine.pOperands) );
        __throwing_func( validateEQULabelFormat(pThis) );
        if (pSymbol)
        {
            /* pSymbol would only be NULL if out of memory was encountered earlier but continued assembly process to
               find and report as many syntax errors as possible to user. */
            pSymbol->expression = expression;
            pThis->pLineInfo->flags |= LINEINFO_FLAG_WAS_EQU;
            updateLinesWhichForwardReferencedThisLabel(pThis, pSymbol);
        }
    }
    __catch
    {
        __nothrow;
    }
}

static void validateEQULabelFormat(Assembler* pThis)
{
    if (!pThis->parsedLine.pLabel)
    {
        LOG_ERROR(pThis, "%s directive requires a line label.", "EQU");
        __throw(invalidArgumentException);
    }
    if (pThis->parsedLine.pLabel[0] == ':')
    {
        LOG_ERROR(pThis, "'%s' can't be a local label when used with EQU.", pThis->parsedLine.pLabel);
        __throw(invalidArgumentException);
    }
}

static void updateLinesWhichForwardReferencedThisLabel(Assembler* pThis, Symbol* pSymbol)
{
    LineInfo* pCurr = NULL;
    
    if (!pSymbol)
        return;
    if (symbolContainsForwardReferences(pSymbol))
        return;
        
    Symbol_LineReferenceEnumStart(pSymbol);
    while (NULL != (pCurr = Symbol_LineReferenceEnumNext(pSymbol)))
    {
        __try
        {
            updateLineWithForwardReference(pThis, pSymbol, pCurr);
        }
        __catch
        {
            LOG_ERROR(pThis, "Failed to allocate space for updating forward references to '%s' symbol.", pSymbol->pKey);
            __nothrow;
        }
    }
}

static int symbolContainsForwardReferences(Symbol* pSymbol)
{
    return pSymbol->expression.flags & EXPRESSION_FLAG_FORWARD_REFERENCE;
}

static void updateLineWithForwardReference(Assembler* pThis, Symbol* pSymbol, LineInfo* pLineInfo)
{
    LineBuffer* pLineBuffer = NULL;
    LineInfo*   pLineInfoSave;
    ParsedLine  parsedLineSave;
    
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
}

static void ignoreOperator(Assembler* pThis)
{
}

static void handleInvalidOperator(Assembler* pThis)
{
    if (pThis->pLineInfo->instructionSet == INSTRUCTION_SET_65816)
    {
        /* UNDONE: snap doesn't currently support this extended instruction set so just emit RTS for all instructions. */
        emitSingleByteInstruction(pThis, 0x60);
        return;
    }

    LOG_ERROR(pThis, "'%s' is not a recognized mnemonic or macro.", pThis->parsedLine.pOperator);
}

static void handleASC(Assembler* pThis)
{
    const char*    pOperands = fullOperandStringWithSpaces(pThis);
    char           delimiter = *pOperands;
    const char*    pCurr = &pOperands[1];
    size_t         i = 0;
    unsigned char  mask = delimiter < '\'' ? 0x80 : 0x00;
    int            alreadyAllocated = isMachineCodeAlreadyAllocatedFromForwardReference(pThis);

    while (*pCurr && *pCurr != delimiter)
    {
        __try
        {
            unsigned char   byte = *pCurr | mask;
            if (!alreadyAllocated)
                __throwing_func( reallocLineInfoMachineCodeBytes(pThis, i+1) );
            pThis->pLineInfo->pMachineCode[i++] = byte;
            pCurr++;
        }
        __catch
        {
            reallocLineInfoMachineCodeBytes(pThis, 0);
            __nothrow;
        }
    }
    assert ( !alreadyAllocated || i == pThis->pLineInfo->machineCodeSize );
    
    if (*pCurr == '\0')
        LOG_ERROR(pThis, "%s didn't end with the expected %c delimiter.", pThis->parsedLine.pOperands, delimiter);
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
    SizedString operands = SizedString_InitFromString(pThis->parsedLine.pOperands);
    Expression  expression;
    
    __try
        expression = getAbsoluteExpression(pThis, &operands);
    __catch
        __nothrow;

    if (!isAlreadyInDUMSection(pThis))
        pThis->programCounterBeforeDUM = pThis->programCounter;
    pThis->programCounter = expression.value;
    pThis->pCurrentBuffer = pThis->pDummyBuffer;
}

static Expression getAbsoluteExpression(Assembler* pThis, SizedString* pOperands)
{
    Expression expression;
    
    __try
    {
        __throwing_func( expression = ExpressionEvalSizedString(pThis, pOperands) );
        if (!isTypeAbsolute(&expression))
        {
            LOG_ERROR(pThis, "'%.*s' doesn't specify an absolute address.", pOperands->stringLength, pOperands->pString);
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
    SizedString   operands = SizedString_InitFromString(pThis->parsedLine.pOperands);
    SizedString   beforeComma;
    SizedString   afterComma;
    Expression    countExpression;
    Expression    fillExpression;
    
    memset(&fillExpression, 0, sizeof(fillExpression));
    SizedString_SplitString(&operands, ',', &beforeComma, &afterComma);
    __try
    {
        __throwing_func( countExpression = getCountExpression(pThis, &beforeComma) );
        if (afterComma.stringLength > 0)
            __throwing_func( fillExpression = getAbsoluteExpression(pThis, &afterComma) );
    }
    __catch
    {
        __nothrow;
    }

    saveDSInfoInLineInfo(pThis, countExpression.value, (unsigned char)fillExpression.value);
}

static Expression getCountExpression(Assembler* pThis, SizedString* pString)
{
    Expression countExpression;
    
    memset(&countExpression, 0, sizeof(countExpression));
    if (0 == SizedString_strcmp(pString, "\\"))
        return getBytesLeftInPage(pThis);
    else
        return getAbsoluteExpression(pThis, pString);
}

static Expression getBytesLeftInPage(Assembler* pThis)
{
    Expression countExpression;
    memset(&countExpression, 0, sizeof(countExpression));
    countExpression.type = TYPE_ABSOLUTE;
    countExpression.value = ((pThis->programCounter & 255) == 0) ? 0 : 256 - (pThis->programCounter & 255);

    return countExpression;
}

static void saveDSInfoInLineInfo(Assembler* pThis, unsigned short repeatCount, unsigned char fillValue)
{
    __try
        allocateLineInfoMachineCodeBytes(pThis, repeatCount);
    __catch
        __nothrow;
    memset(pThis->pLineInfo->pMachineCode, fillValue, repeatCount);
}

static void handleHEX(Assembler* pThis)
{
    const char*    pCurr = pThis->parsedLine.pOperands;
    size_t         i = 0;
    int            alreadyAllocated = isMachineCodeAlreadyAllocatedFromForwardReference(pThis);

    while (*pCurr && i < 32)
    {
        __try
        {
            unsigned int   byte;
            const char*    pNext;

            __throwing_func( byte = getNextHexByte(pCurr, &pNext) );
            if (!alreadyAllocated)
                __throwing_func( reallocLineInfoMachineCodeBytes(pThis, i+1) );
            pThis->pLineInfo->pMachineCode[i++] = byte;
            pCurr = pNext;
        }
        __catch
        {
            logHexParseError(pThis);
            reallocLineInfoMachineCodeBytes(pThis, 0);
            __nothrow;
        }
    }
    assert ( !alreadyAllocated || i == pThis->pLineInfo->machineCodeSize );
    
    if (*pCurr)
    {
        LOG_ERROR(pThis, "'%s' contains more than 32 values.", pThis->parsedLine.pOperands);
        return;
    }
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
        return value - 'A' + 10;
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
    SizedString operands = SizedString_InitFromString(pThis->parsedLine.pOperands);
    Expression  expression;
    
    __try
    {
        __throwing_func( expression = getAbsoluteExpression(pThis, &operands) );
        setOrgInAssemblerAndBinaryBufferModules(pThis, expression.value);
    }
    __catch
    {
        __nothrow;
    }
}

static void handleSAV(Assembler* pThis)
{
    __try
    {
        BinaryBuffer_QueueWriteToFile(pThis->pObjectBuffer, pThis->parsedLine.pOperands);
    }
    __catch
    {
        LOG_ERROR(pThis, "Failed to queue up save to '%s'.", pThis->parsedLine.pOperands);
        __nothrow;
    }
}

static void handleDB(Assembler* pThis)
{
    size_t      i = 0;
    SizedString nextOperands = SizedString_InitFromString(pThis->parsedLine.pOperands);
    int         alreadyAllocated = isMachineCodeAlreadyAllocatedFromForwardReference(pThis);

    while (nextOperands.stringLength != 0)
    {
        Expression  expression;
        SizedString beforeComma;
        SizedString afterComma;

        __try
        {
            SizedString_SplitString(&nextOperands, ',', &beforeComma, &afterComma);
            __throwing_func( expression = ExpressionEvalSizedString(pThis, &beforeComma) );
            if (!alreadyAllocated)
                __throwing_func( reallocLineInfoMachineCodeBytes(pThis, i + 1) );
            pThis->pLineInfo->pMachineCode[i++] = (unsigned char)expression.value;
            nextOperands = afterComma;
        }
        __catch
        {
            reallocLineInfoMachineCodeBytes(pThis, 0);
            __nothrow;
        }
    }
    assert ( !alreadyAllocated || i == pThis->pLineInfo->machineCodeSize );
}

static void handleDA(Assembler* pThis)
{
    size_t      i = 0;
    SizedString nextOperands = SizedString_InitFromString(pThis->parsedLine.pOperands);
    int         alreadyAllocated = isMachineCodeAlreadyAllocatedFromForwardReference(pThis);

    while (nextOperands.stringLength != 0)
    {
        Expression  expression;
        SizedString beforeComma;
        SizedString afterComma;

        __try
        {
            SizedString_SplitString(&nextOperands, ',', &beforeComma, &afterComma);
            __throwing_func( expression = ExpressionEvalSizedString(pThis, &beforeComma) );
            if (!alreadyAllocated)
                __throwing_func( reallocLineInfoMachineCodeBytes(pThis, i+2) );
            pThis->pLineInfo->pMachineCode[i++] = (unsigned char)expression.value;
            pThis->pLineInfo->pMachineCode[i++] = (unsigned char)(expression.value >> 8);
            nextOperands = afterComma;
        }
        __catch
        {
            reallocLineInfoMachineCodeBytes(pThis, 0);
            __nothrow;
        }
    }
    assert ( !alreadyAllocated || i == pThis->pLineInfo->machineCodeSize );
}

static void handleXC(Assembler* pThis)
{
    if (pThis->parsedLine.pOperands && 0 == strcasecmp(pThis->parsedLine.pOperands, "OFF"))
    {
        pThis->instructionSet = INSTRUCTION_SET_6502;
        return;
    }
    
    pThis->instructionSet++;
    if (pThis->instructionSet == INSTRUCTION_SET_INVALID)
    {
        LOG_ERROR(pThis, "Can't have more than 2 %s directives.", "XC");
        pThis->instructionSet--;
    }
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
    outputListFile(pThis);    
    BinaryBuffer_ProcessWriteFileQueue(pThis->pObjectBuffer);
}

static void outputListFile(Assembler* pThis)
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
