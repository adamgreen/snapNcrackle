/*  Copyright (C) 2013  Adam Green (https://github.com/adamgreen)
    Copyright (C) 2013  Tee-Kiah Chia

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <stdlib.h>
#include "AssemblerPriv.h"
#include "ExpressionEval.h"
#include "AddressingMode.h"
#include "InstructionSets.h"
#include "TextFileSource.h"
#include "LupSource.h"


static void commonObjectInit(Assembler* pThis, const AssemblerInitParams* pParams, TextFile* pTextFile);
static FILE* createListFileOrRedirectToStdOut(Assembler* pThis, const AssemblerInitParams* pParams);
static void createParseObjectForPutSearchPath(Assembler* ptThis, const AssemblerInitParams* pParams);
static void createFullInstructionSetTables(Assembler* pThis); 
static void create6502InstructionSetTable(Assembler* pThis);
static int compareInstructionSetEntries(const void* pv1, const void* pv2);
static void create65c02InstructionSetTable(Assembler* pThis);
static void create65816InstructionSetTable(Assembler* pThis);
static size_t countNewInstructionMnemonics(const OpCodeEntry* pBaseSet, size_t baseSetLength, 
                                           const OpCodeEntry* pNewSet,  size_t newSetLength);
static int compareInstructionSetEntryToOperatorString(const void* pvKey, const void* pvEntry);
static void mergeInstructionSets(OpCodeEntry* pBaseSet, size_t baseSetLength, size_t totalLength,
                                       const OpCodeEntry* pAddSet,  size_t addSetLength);
static void updateInstructionEntry(OpCodeEntry* pEntryToUpdate, const OpCodeEntry* pAdditionalEntry);
static void initParameterVariablesTo0(Assembler* pThis);
static void initParameterVariableTo0(Assembler* pThis, const char* pVariableName);
static void setOrgInAssemblerAndBinaryBufferModules(Assembler* pThis, unsigned short orgAddress);
__throws Assembler* Assembler_CreateFromString(const char* pText, const AssemblerInitParams* pParams)
{
    Assembler* pThis = NULL;
    
    __try
    {
        TextFile* pTextFile;
        
        pThis = allocateAndZero(sizeof(*pThis));
        pTextFile = TextFile_CreateFromString(pText);
        commonObjectInit(pThis, pParams, pTextFile);
    }
    __catch
    {
        Assembler_Free(pThis);
        __rethrow;
    }

    return pThis;
}

static void commonObjectInit(Assembler* pThis, const AssemblerInitParams* pParams, TextFile* pTextFile)
{
    __try
    {
        FILE* pListFile;
        
        TextSource* pTextSource = TextFileSource_Create(pTextFile);
        pTextFile = NULL;
        TextSource_StackPush(&pThis->pTextSourceStack, pTextSource);
        pThis->linesHead.pTextSource = pTextSource;
        pListFile = createListFileOrRedirectToStdOut(pThis, pParams);
        pThis->pListFile = ListFile_Create(pListFile);
        pThis->pSymbols = SymbolTable_Create(NUMBER_OF_SYMBOL_TABLE_HASH_BUCKETS);
        pThis->pObjectBuffer = BinaryBuffer_Create(SIZE_OF_OBJECT_AND_DUMMY_BUFFERS);
        pThis->pDummyBuffer = BinaryBuffer_Create(SIZE_OF_OBJECT_AND_DUMMY_BUFFERS);
        createParseObjectForPutSearchPath(pThis, pParams);
        createFullInstructionSetTables(pThis);
        pThis->pInitParams = pParams;
        pThis->pLineInfo = &pThis->linesHead;
        pThis->pCurrentBuffer = pThis->pObjectBuffer;
        setOrgInAssemblerAndBinaryBufferModules(pThis, 0x8000);
        initParameterVariablesTo0(pThis);
    }
    __catch
    {
        TextFile_Free(pTextFile);
        __rethrow;
    }
}

static FILE* createListFileOrRedirectToStdOut(Assembler* pThis, const AssemblerInitParams* pParams)
{
    if (!pParams || !pParams->pListFilename)
        return stdout;
        
    pThis->pFileForListing = fopen(pParams->pListFilename, "wb");
    if (!pThis->pFileForListing)
        __throw(fileOpenException);
    return pThis->pFileForListing;
}

static void createParseObjectForPutSearchPath(Assembler* pThis, const AssemblerInitParams* pParams)
{
    ParseCSV* pParser = NULL;
    
    if (!pParams || !pParams->pPutDirectories)
        return;
    
    __try
    {
        SizedString putDirectories = SizedString_InitFromString(pParams->pPutDirectories);
        pParser = ParseCSV_CreateWithCustomSeparator(';');
        ParseCSV_Parse(pParser, &putDirectories);
    }
    __catch
    {
        ParseCSV_Free(pParser);
        __rethrow;
    }
    
    pThis->pPutSearchPath = pParser;
}

static void createFullInstructionSetTables(Assembler* pThis)
{
    create6502InstructionSetTable(pThis);
    create65c02InstructionSetTable(pThis);
    create65816InstructionSetTable(pThis);
}

static void create6502InstructionSetTable(Assembler* pThis)
{
    pThis->instructionSets[INSTRUCTION_SET_6502] = allocateAndZero(sizeof(g_6502InstructionSet));
    memcpy(pThis->instructionSets[INSTRUCTION_SET_6502], g_6502InstructionSet, sizeof(g_6502InstructionSet));
    pThis->instructionSetSizes[INSTRUCTION_SET_6502] = ARRAYSIZE(g_6502InstructionSet);
    qsort(pThis->instructionSets[INSTRUCTION_SET_6502], ARRAYSIZE(g_6502InstructionSet), sizeof(g_6502InstructionSet[0]), 
          compareInstructionSetEntries);
}

static int compareInstructionSetEntries(const void* pv1, const void* pv2)
{
    OpCodeEntry* p1 = (OpCodeEntry*)pv1;
    OpCodeEntry* p2 = (OpCodeEntry*)pv2;
    
    return strcasecmp(p1->pOperator, p2->pOperator);
}

static void create65c02InstructionSetTable(Assembler* pThis)
{
    size_t baseSetLength = pThis->instructionSetSizes[INSTRUCTION_SET_6502];
    size_t instructionsAdded = countNewInstructionMnemonics(
                                    pThis->instructionSets[INSTRUCTION_SET_6502], baseSetLength, 
                                    g_65c02AdditionalInstructions, ARRAYSIZE(g_65c02AdditionalInstructions));
    size_t newSetLength = baseSetLength + instructionsAdded;
    
    pThis->instructionSets[INSTRUCTION_SET_65C02] = allocateAndZero(sizeof(OpCodeEntry) * newSetLength);
    memcpy(pThis->instructionSets[INSTRUCTION_SET_65C02], pThis->instructionSets[INSTRUCTION_SET_6502], 
           sizeof(OpCodeEntry) * baseSetLength);
    mergeInstructionSets(pThis->instructionSets[INSTRUCTION_SET_65C02], baseSetLength, newSetLength, 
                               g_65c02AdditionalInstructions, ARRAYSIZE(g_65c02AdditionalInstructions));
    pThis->instructionSetSizes[INSTRUCTION_SET_65C02] = newSetLength;
    qsort(pThis->instructionSets[INSTRUCTION_SET_65C02], newSetLength, sizeof(g_6502InstructionSet[0]), compareInstructionSetEntries);
}

static void create65816InstructionSetTable(Assembler* pThis)
{
    size_t baseSetLength = pThis->instructionSetSizes[INSTRUCTION_SET_65C02];
    size_t instructionsAdded = countNewInstructionMnemonics(
                                    pThis->instructionSets[INSTRUCTION_SET_65C02], baseSetLength, 
                                    g_65816AdditionalInstructions, ARRAYSIZE(g_65816AdditionalInstructions));
    size_t newSetLength = baseSetLength + instructionsAdded;
    
    pThis->instructionSets[INSTRUCTION_SET_65816] = allocateAndZero(sizeof(OpCodeEntry) * newSetLength);
    memcpy(pThis->instructionSets[INSTRUCTION_SET_65816], pThis->instructionSets[INSTRUCTION_SET_65C02], 
           sizeof(OpCodeEntry) * baseSetLength);
    mergeInstructionSets(pThis->instructionSets[INSTRUCTION_SET_65816], baseSetLength, newSetLength, 
                               g_65816AdditionalInstructions, ARRAYSIZE(g_65816AdditionalInstructions));
    pThis->instructionSetSizes[INSTRUCTION_SET_65816] = newSetLength;
    qsort(pThis->instructionSets[INSTRUCTION_SET_65816], newSetLength, sizeof(g_6502InstructionSet[0]), compareInstructionSetEntries);
}

static size_t countNewInstructionMnemonics(const OpCodeEntry* pBaseSet, size_t baseSetLength, 
                                           const OpCodeEntry* pNewSet,  size_t newSetLength)
{
    size_t count = 0;
    size_t i;
    
    for (i = 0 ; i < newSetLength ; i++)
    {
        if (NULL == bsearch(pNewSet[i].pOperator, 
                            pBaseSet, baseSetLength, sizeof(*pBaseSet), 
                            compareInstructionSetEntryToOperatorString))
        {
            count++;
        }
    }
    return count;
}

static int compareInstructionSetEntryToOperatorString(const void* pvKey, const void* pvEntry)
{
    const char* pKey = (const char*)pvKey;
    const OpCodeEntry* pEntry = (OpCodeEntry*)pvEntry;
    
    return strcasecmp(pKey, pEntry->pOperator);
}

static void mergeInstructionSets(OpCodeEntry* pBaseSet, size_t baseSetLength, size_t totalLength,
                                 const OpCodeEntry* pAddSet,  size_t addSetLength)
{
    OpCodeEntry* pNewInstruction = pBaseSet + baseSetLength;
    size_t       i;
    
    for (i = 0 ; i < addSetLength ; i++)
    {
        OpCodeEntry* pExistingEntry = bsearch(pAddSet[i].pOperator, 
                                              pBaseSet, baseSetLength, sizeof(*pBaseSet), 
                                              compareInstructionSetEntryToOperatorString);
        if (pExistingEntry)
            updateInstructionEntry(pExistingEntry, &pAddSet[i]);
        else
            *pNewInstruction++ = pAddSet[i];
    }
    assert ( pNewInstruction - pBaseSet == (long)totalLength );
}

#define COPY_UPDATED_FIELD(FIELD) if (pAdditionalEntry->FIELD != _xXX) pEntryToUpdate->FIELD = pAdditionalEntry->FIELD

static void updateInstructionEntry(OpCodeEntry* pEntryToUpdate, const OpCodeEntry* pAdditionalEntry)
{
    COPY_UPDATED_FIELD(opcodeImmediate);
    COPY_UPDATED_FIELD(opcodeAbsolute);
    COPY_UPDATED_FIELD(opcodeZeroPage);
    COPY_UPDATED_FIELD(opcodeImplied);
    COPY_UPDATED_FIELD(opcodeZeroPageIndexedIndirect);
    COPY_UPDATED_FIELD(opcodeIndirectIndexed);
    COPY_UPDATED_FIELD(opcodeZeroPageIndexedX);
    COPY_UPDATED_FIELD(opcodeZeroPageIndexedY);
    COPY_UPDATED_FIELD(opcodeAbsoluteIndexedX);
    COPY_UPDATED_FIELD(opcodeAbsoluteIndexedY);
    COPY_UPDATED_FIELD(opcodeRelative);
    COPY_UPDATED_FIELD(opcodeAbsoluteIndirect);
    COPY_UPDATED_FIELD(opcodeAbsoluteIndexedIndirect);
    COPY_UPDATED_FIELD(opcodeZeroPageIndirect);
    pEntryToUpdate->longImmediateIfLongA = pAdditionalEntry->longImmediateIfLongA;
    pEntryToUpdate->longImmediateIfLongXY = pAdditionalEntry->longImmediateIfLongXY;
}

static void initParameterVariablesTo0(Assembler* pThis)
{
    initParameterVariableTo0(pThis, "]0");
    initParameterVariableTo0(pThis, "]1");
    initParameterVariableTo0(pThis, "]2");
    initParameterVariableTo0(pThis, "]3");
    initParameterVariableTo0(pThis, "]4");
    initParameterVariableTo0(pThis, "]5");
    initParameterVariableTo0(pThis, "]6");
    initParameterVariableTo0(pThis, "]7");
    initParameterVariableTo0(pThis, "]8");
    initParameterVariableTo0(pThis, "]9");
}

static void initParameterVariableTo0(Assembler* pThis, const char* pVariableName)
{
    SizedString globalVariableName = SizedString_InitFromString(pVariableName);
    SizedString nullLocalName = SizedString_InitFromString(NULL);
    Symbol* pSymbol = SymbolTable_Add(pThis->pSymbols, &globalVariableName, &nullLocalName);
    pSymbol->pDefinedLine = &pThis->linesHead;
    pSymbol->expression = ExpressionEval_CreateAbsoluteExpression(0);
}

static void setOrgInAssemblerAndBinaryBufferModules(Assembler* pThis, unsigned short orgAddress)
{
    pThis->programCounter = orgAddress;
    BinaryBuffer_SetOrigin(pThis->pCurrentBuffer, orgAddress);
}


__throws Assembler* Assembler_CreateFromFile(const char* pSourceFilename, const AssemblerInitParams* pParams)
{
    Assembler*  pThis = NULL;
    
    __try
    {
        TextFile* pTextFile;
        
        SizedString sourceFilename = SizedString_InitFromString(pSourceFilename);
        pThis = allocateAndZero(sizeof(*pThis));
        pTextFile = TextFile_CreateFromFile(NULL, &sourceFilename, NULL);
        commonObjectInit(pThis, pParams, pTextFile);
    }
    __catch
    {
        Assembler_Free(pThis);
        __rethrow;
    }

    return pThis;
}


static void freeLines(Assembler* pThis);
static void freeConditionals(Assembler* pThis);
static void freeInstructionSets(Assembler* pThis);
void Assembler_Free(Assembler* pThis)
{
    if (!pThis)
        return;
    
    freeLines(pThis);
    freeConditionals(pThis);
    freeInstructionSets(pThis);
    ParseCSV_Free(pThis->pPutSearchPath);
    ListFile_Free(pThis->pListFile);
    BinaryBuffer_Free(pThis->pDummyBuffer);
    BinaryBuffer_Free(pThis->pObjectBuffer);
    SymbolTable_Free(pThis->pSymbols);
    TextSource_FreeAll();
    if (pThis->pFileForListing)
        fclose(pThis->pFileForListing);
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

static void freeConditionals(Assembler* pThis)
{
    Conditional* pCurr = pThis->pConditionals;
    
    while (pCurr)
    {
        Conditional* pPrev = pCurr->pPrev;
        free(pCurr);
        pCurr = pPrev;
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
static int getNextSourceLine(Assembler* pThis, SizedString* pLine);
static int attemptToPopTextFileAndGetNextLine(Assembler* pThis, SizedString* pLine);
static void parseLine(Assembler* pThis, const SizedString* pLine);
static int shouldSkipSourceLines(Assembler* pThis);
static void prepareLineInfoForThisLine(Assembler* pThis, const SizedString* pLine);
static void rememberLabelIfGlobal(Assembler* pThis);
static int doesLineContainALabel(Assembler* pThis);
static int isGlobalLabelName(SizedString* pLabelName);
static int isLocalLabelName(SizedString* pLabelName);
static int isVariableLabelName(SizedString* pLabelName);
static void addUnhandledLabel(Assembler* pThis, unsigned short addressForLabel);
static int hasLabelAlreadyBeenDefined(Assembler* pThis);
static Symbol* attemptToAddSymbol(Assembler* pThis, SizedString* pLabelName, Expression* pExpression);
static SizedString initGlobalLabelString(Assembler* pThis, SizedString* pLabelName);
static SizedString initLocalLabelString(SizedString* pLabelName);
static void validateLabelFormat(Assembler* pThis, SizedString* pLabel);
static int seenGlobalLabel(Assembler* pThis);
static int isSymbolAlreadyDefined(Symbol* pSymbol, LineInfo* pThisLine);
static void flagSymbolAsDefined(Symbol* pSymbol, LineInfo* pThisLine);
static void firstPassAssembleLine(Assembler* pThis);
static int compareInstructionSetEntryToOperatorSizedString(const void* pvKey, const void* pvEntry);
static void handleOpcode(Assembler* pThis, const OpCodeEntry* pOpcodeEntry);
static int isOpcodeSkippable(const OpCodeEntry* pOpcodeEntry);
static void handleImpliedAddressingMode(Assembler* pThis, unsigned char opcodeImplied);
static void logInvalidAddressingModeError(Assembler* pThis);
static void emitSingleByteInstruction(Assembler* pThis, unsigned char opCode);
static void allocateLineInfoMachineCodeBytes(Assembler* pThis, size_t bytesToAllocate);
static int isMachineCodeAlreadyAllocatedFromForwardReference(Assembler* pThis);
static void verifyThatMachineCodeSizeFromForwardReferenceMatches(Assembler* pThis, size_t bytesToAllocate);
static void reallocLineInfoMachineCodeBytes(Assembler* pThis, size_t bytesToAllocate);
static void handleZeroPageAbsoluteOrRelativeAddressingMode(Assembler*         pThis, 
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
static void emitFourByteInstruction(Assembler* pThis, unsigned char opCode, uint32_t value);
static void handleShortImmediateAddressingMode(Assembler*      pThis, 
                                               AddressingMode* pAddressingMode, 
                                               unsigned char   opcodeImmediate);
static void handleLongImmediateAddressingMode(Assembler*      pThis, 
                                             AddressingMode* pAddressingMode, 
                                             unsigned char   opcodeImmediate);
static void handleTwoByteAddressingMode(Assembler*      pThis, 
                                        AddressingMode* pAddressingMode, 
                                        unsigned char   opcode);
static void handleThreeByteAddressingMode(Assembler*      pThis, 
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
static void validateOperandWasProvided(Assembler* pThis);
static void validateEQULabelFormat(Assembler* pThis);
static void updateLinesWhichForwardReferencedThisLabel(Assembler* pThis, Symbol* pSymbol);
static int symbolContainsForwardReferences(Symbol* pSymbol);
static void updateLineWithForwardReference(Assembler* pThis, Symbol* pSymbol, LineInfo* pLineInfo);
static void flagLineInfoAsProcessingForwardReference(LineInfo* pLineInfo);
static void resetLineInfoAsNotProcessingForwardReference(LineInfo* pLineInfo);
static void handleInvalidOperator(Assembler* pThis);
static SizedString fullOperandStringWithSpaces(Assembler* pThis);
static void reverseMachineCode(LineInfo* pLineInfo);
static Expression getAbsoluteExpression(Assembler* pThis, SizedString* pOperands);
static int isTypeAbsolute(Expression* pExpression);
static int isAlreadyInDUMSection(Assembler* pThis);
static void warnIfOperandWasProvided(Assembler* pThis);
static Expression getCountExpression(Assembler* pThis, SizedString* pString);
static Expression getBytesLeftInPage(Assembler* pThis);
static void saveDSInfoInLineInfo(Assembler* pThis, unsigned short repeatCount, unsigned char fillValue);
static void parseHexData(Assembler* pThis, const SizedString* pOperands, const char** ppCurr, int alreadyAllocated, size_t i);
static unsigned char getNextHexByte(const SizedString* pString, const char** ppCurr);
static unsigned char hexCharToNibble(char value);
static void logHexParseError(Assembler* pThis);
static TextFile* openPutFileUsingSearchPath(Assembler* pThis, const SizedString* pFilename);
#if 0
static int isProcessingTextFromPutFile(Assembler* pThis);
#endif
static SizedString removeDirectoryAndSuffixFromFullFilename(SizedString* pFullFilename);
static SizedString getNextCommaSeparatedOptionalStringArgument(Assembler* pThis, SizedString* pRemainingArguments);
static SizedString getNextCommaSeparatedStringArgument(Assembler* pThis, SizedString* pRemainingArguments);
static uint32_t getNextCommaSeparatedLongArgument(Assembler* pThis, SizedString* pRemainingArguments);
static unsigned short getNextCommaSeparatedArgument(Assembler* pThis, SizedString* pRemainingArguments);
static unsigned short getNextCommaSeparatedOptionalArgument(Assembler* pThis, SizedString* pRemainingArguments, unsigned short defaultValue);
static int isUpdatingForwardReference(Assembler* pThis);
static void disallowForwardReferences(Assembler* pThis);
static int doesExpressionEqualZeroIndicatingToSkipSourceLines(Expression* pExpression);
static void pushConditional(Assembler* pThis, int skipSourceLines);
static unsigned int determineInheritedConditionalSkipSourceLineState(Assembler* pThis);
static void flipTopConditionalState(Assembler* pThis);
static void validateInConditionalAlready(Assembler* pThis);
static void flipConditionalState(Conditional* pConditional);
static void validateElseNotAlreadySeen(Assembler* pThis);
static int seenElseAlready(Conditional* pConditional);
static void rememberThatConditionalHasSeenElse(Conditional* pConditional);
static void popConditional(Assembler* pThis);
static void flagThatLupDirectiveHasBeenSeen(Assembler* pThis);
static void validateLupExpressionInRange(Assembler* pThis, Expression* pExpression);
static void validateThatLupEndWasFound(Assembler* pThis, ParsedLine* pParsedLine);
static int haveSeenLupDirective(Assembler* pThis);
static void clearLupDirectiveFlag(Assembler* pThis);
static void checkForUndefinedSymbols(Assembler* pThis);
static void checkSymbolForOutstandingForwardReferences(Assembler* pThis, Symbol* pSymbol);
static void checkForOpenConditionals(Assembler* pThis);
static void secondPass(Assembler* pThis);
static void outputListFile(Assembler* pThis);
void Assembler_Run(Assembler* pThis)
{
    firstPass(pThis);
    checkForUndefinedSymbols(pThis);
    checkForOpenConditionals(pThis);
    secondPass(pThis);
}

static void firstPass(Assembler* pThis)
{
    SizedString line;
    while (getNextSourceLine(pThis, &line))
        parseLine(pThis, &line);
}

static int getNextSourceLine(Assembler* pThis, SizedString* pLine)
{
    if (TextSource_IsEndOfFile(pThis->pTextSourceStack))
        return attemptToPopTextFileAndGetNextLine(pThis, pLine);

    *pLine = TextSource_GetNextLine(pThis->pTextSourceStack);
    return 1;
}

static int attemptToPopTextFileAndGetNextLine(Assembler* pThis, SizedString* pLine)
{
    TextSource_StackPop(&pThis->pTextSourceStack);
    if (!pThis->pTextSourceStack)
        return 0;

    if (TextSource_IsEndOfFile(pThis->pTextSourceStack))
        return 0;

    *pLine = TextSource_GetNextLine(pThis->pTextSourceStack);
    return 1;
}

static void parseLine(Assembler* pThis, const SizedString* pLine)
{
    /* The PoP code contains constructs like

          bra PalFade
          ...
         PalFade dum 0
         :green ds 1
         :blue ds 1
          dend
          ...

       In such cases, a DUM appears on the same line as a label, and the
       correct behaviour is to assign to the label the assembly's program
       counter _before_ the DUM, not after.

       -- tkchia 20131012
     */
    unsigned short originalProgramCounter = pThis->programCounter;
    prepareLineInfoForThisLine(pThis, pLine);
    ParseLine(&pThis->parsedLine, pLine);
    rememberLabelIfGlobal(pThis);
    firstPassAssembleLine(pThis);
    if (!shouldSkipSourceLines(pThis))
        addUnhandledLabel(pThis, originalProgramCounter);
    pThis->programCounter += pThis->pLineInfo->machineCodeSize;
}

static int shouldSkipSourceLines(Assembler* pThis)
{
    return (int)(pThis->pLineInfo->flags & CONDITIONAL_SKIP_STATES_MASK);
}

static void prepareLineInfoForThisLine(Assembler* pThis, const SizedString* pLine)
{
    LineInfo* pLineInfo = allocateAndZero(sizeof(*pLineInfo));
    pLineInfo->pTextSource = pThis->pTextSourceStack;
    pLineInfo->lineNumber = TextSource_GetLineNumber(pThis->pTextSourceStack);
    pLineInfo->lineText = *pLine;
    pLineInfo->address = pThis->programCounter;
    pLineInfo->instructionSet = pThis->instructionSet;
    pLineInfo->indentation = (TextSource_StackDepth(pThis->pTextSourceStack)-1) * 4;
    pLineInfo->flags = pThis->pConditionals ? pThis->pConditionals->flags & CONDITIONAL_SKIP_STATES_MASK : 0;
    pThis->pLineInfo->pNext = pLineInfo;
    pThis->pLineInfo = pLineInfo;
}

static void rememberLabelIfGlobal(Assembler* pThis)
{
    if (!doesLineContainALabel(pThis) || shouldSkipSourceLines(pThis) || !isGlobalLabelName(&pThis->parsedLine.label))
        return;
        
    pThis->globalLabel = pThis->parsedLine.label;
}

static int doesLineContainALabel(Assembler* pThis)
{
    return SizedString_strlen(&pThis->parsedLine.label) > 0;
}

static int isGlobalLabelName(SizedString* pLabelName)
{
    return !isLocalLabelName(pLabelName) && !isVariableLabelName(pLabelName);
}

static int isLocalLabelName(SizedString* pLabelName)
{
    return pLabelName->pString && pLabelName->pString[0] == ':';
}

static int isVariableLabelName(SizedString* pLabelName)
{
    return pLabelName->pString && pLabelName->pString[0] == ']';
}

static void addUnhandledLabel(Assembler* pThis, unsigned short addressForLabel)
{
    Expression  expression;

    if (!doesLineContainALabel(pThis) || hasLabelAlreadyBeenDefined(pThis))
        return;

    __try
    {
        expression = ExpressionEval_CreateAbsoluteExpression(addressForLabel);
        attemptToAddSymbol(pThis, &pThis->parsedLine.label, &expression);
    }
    __catch
    {
        __nothrow;
    }
}

static int hasLabelAlreadyBeenDefined(Assembler* pThis)
{
    return pThis->pLineInfo->flags & LINEINFO_FLAG_WAS_EQU;
}

static Symbol* attemptToAddSymbol(Assembler* pThis, SizedString* pLabelName, Expression* pExpression)
{
    SizedString globalLabel = initGlobalLabelString(pThis, pLabelName);
    SizedString localLabel = initLocalLabelString(pLabelName);
    
    validateLabelFormat(pThis, &pThis->parsedLine.label);
    Symbol* pSymbol = SymbolTable_Find(pThis->pSymbols, &globalLabel, &localLabel);
    if (pSymbol && (isSymbolAlreadyDefined(pSymbol, pThis->pLineInfo) && !isVariableLabelName(pLabelName)))
    {
        LOG_ERROR(pThis, "'%.*s%.*s' symbol has already been defined.", 
                  pThis->globalLabel.stringLength, pThis->globalLabel.pString,
                  localLabel.stringLength, localLabel.pString);
        __throw(invalidArgumentException);
    }
    if (!pSymbol)
    {
        __try
        {
            pSymbol = SymbolTable_Add(pThis->pSymbols, &globalLabel, &localLabel);
        }
        __catch
        {
            LOG_ERROR(pThis, "Failed to allocate space for '%.*s%.*s' symbol.",
                      pThis->globalLabel.stringLength, pThis->globalLabel.pString,
                      localLabel.stringLength, localLabel.pString);
            __rethrow;
        }
    }
    flagSymbolAsDefined(pSymbol, pThis->pLineInfo);
    pSymbol->expression = *pExpression;
    updateLinesWhichForwardReferencedThisLabel(pThis, pSymbol);

    return pSymbol;
}

static SizedString initGlobalLabelString(Assembler* pThis, SizedString* pLabelName)
{
    if (isLocalLabelName(pLabelName))
        return pThis->globalLabel;

    return *pLabelName;
}

static SizedString initLocalLabelString(SizedString* pLabelName)
{
    if (isLocalLabelName(pLabelName))
        return *pLabelName;
    return SizedString_InitFromString(NULL);
}

static void validateLabelFormat(Assembler* pThis, SizedString* pLabel)
{
    const char*  pCurr;
    char         ch;
    
    SizedString_EnumStart(pLabel, &pCurr);
    if (SizedString_EnumNext(pLabel, &pCurr) < ':')
    {
        LOG_ERROR(pThis, "'%.*s' label starts with invalid character.", 
                  pLabel->stringLength, pLabel->pString);
        __throw(invalidArgumentException);
    }
    
    while ((ch = SizedString_EnumNext(pLabel, &pCurr)) != '\0')
    {
        if (ch < '0')
        {
            LOG_ERROR(pThis, "'%.*s' label contains invalid character, '%c'.", 
                      pLabel->stringLength, pLabel->pString,
                      ch);
            __throw(invalidArgumentException);
        }
    }
    
    if (isLocalLabelName(pLabel) && !seenGlobalLabel(pThis))
    {
        LOG_ERROR(pThis, "'%.*s' local label isn't allowed before first global label.", 
                  pLabel->stringLength, pLabel->pString);
        __throw(invalidArgumentException);
    }
}

static int seenGlobalLabel(Assembler* pThis)
{
    return SizedString_strlen(&pThis->globalLabel) > 0;
}

static int isSymbolAlreadyDefined(Symbol* pSymbol, LineInfo* pThisLine)
{
    return pSymbol->pDefinedLine != NULL && pSymbol->pDefinedLine != pThisLine;
}

static void flagSymbolAsDefined(Symbol* pSymbol, LineInfo* pThisLine)
{
    pSymbol->pDefinedLine = pThisLine;
    pThisLine->pSymbol = pSymbol;
}

static void firstPassAssembleLine(Assembler* pThis)
{
    const OpCodeEntry* pInstructionSet = pThis->instructionSets[pThis->pLineInfo->instructionSet];
    size_t             instructionSetSize = pThis->instructionSetSizes[pThis->pLineInfo->instructionSet];
    SizedString*       pOperator = &pThis->parsedLine.op;
    const OpCodeEntry* pFoundEntry;
    
    if (SizedString_strlen(pOperator) == 0)
        return;
    
    pFoundEntry = bsearch(pOperator, 
                          pInstructionSet, instructionSetSize, sizeof(*pInstructionSet), 
                          compareInstructionSetEntryToOperatorSizedString);
    if (pFoundEntry)
        handleOpcode(pThis, pFoundEntry);
    else
        handleInvalidOperator(pThis);
}

static int compareInstructionSetEntryToOperatorSizedString(const void* pvKey, const void* pvEntry)
{
    const SizedString* pKey = (const SizedString*)pvKey;
    const OpCodeEntry* pEntry = (OpCodeEntry*)pvEntry;
    
    return SizedString_strcasecmp(pKey, pEntry->pOperator);
}

static void handleOpcode(Assembler* pThis, const OpCodeEntry* pOpcodeEntry)
{
    AddressingMode addressingMode;

    if (shouldSkipSourceLines(pThis) && isOpcodeSkippable(pOpcodeEntry))
        return;
        
    if (pOpcodeEntry->directiveHandler)
    {
        pOpcodeEntry->directiveHandler(pThis);
        return;
    }
    
    __try
        addressingMode = AddressingMode_Eval(pThis, &pThis->parsedLine.operands);
    __catch
        __nothrow;
        
    switch (addressingMode.mode)
    {
    /* Invalid mode gets caught in try/catch block above but placing here silences compiler warning and keeps 
       100% code coverage. */
    default:
    case ADDRESSING_MODE_INVALID:
    case ADDRESSING_MODE_ABSOLUTE:
        handleZeroPageAbsoluteOrRelativeAddressingMode(pThis, &addressingMode, pOpcodeEntry);
        break;
    case ADDRESSING_MODE_IMMEDIATE:
        if ((pThis->longA && pOpcodeEntry->longImmediateIfLongA) ||
            (pThis->longXY && pOpcodeEntry->longImmediateIfLongXY))
            handleLongImmediateAddressingMode(pThis, &addressingMode, pOpcodeEntry->opcodeImmediate);
        else
            handleShortImmediateAddressingMode(pThis, &addressingMode, pOpcodeEntry->opcodeImmediate);
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

static int isOpcodeSkippable(const OpCodeEntry* pOpcodeEntry)
{
    return pOpcodeEntry->directiveHandler != handleELSE &&
           pOpcodeEntry->directiveHandler != handleDO &&
           pOpcodeEntry->directiveHandler != handleFIN;
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
    LOG_ERROR(pThis, "Addressing mode of '%.*s' is not supported for '%.*s' instruction.", 
              pThis->parsedLine.operands.stringLength, pThis->parsedLine.operands.pString, 
              pThis->parsedLine.op.stringLength, pThis->parsedLine.op.pString);
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
        LOG_ERROR(pThis, "Couldn't properly infer size of a forward reference in '%.*s' operand.", 
                  pThis->parsedLine.operands.stringLength, pThis->parsedLine.operands.pString);
        __throw(invalidArgumentException);
    }
}

static void reallocLineInfoMachineCodeBytes(Assembler* pThis, size_t bytesToAllocate)
{
    __try
    {
        pThis->pLineInfo->pMachineCode = BinaryBuffer_Realloc(pThis->pCurrentBuffer, 
                                                              pThis->pLineInfo->pMachineCode, 
                                                              bytesToAllocate);
        pThis->pLineInfo->machineCodeSize = bytesToAllocate;
    }
    __catch
    {
        LOG_ERROR(pThis, "Exceeded the %d allowed bytes in the object file.", SIZE_OF_OBJECT_AND_DUMMY_BUFFERS);
        pThis->pLineInfo->machineCodeSize = 0;
        __rethrow;
    }
}

static void handleZeroPageAbsoluteOrRelativeAddressingMode(Assembler*         pThis, 
                                                           AddressingMode*    pAddressingMode, 
                                                           const OpCodeEntry* pOpcodeEntry)
{
    if (pOpcodeEntry->opcodeRelative != _xXX && pOpcodeEntry->opcodeZeroPage != _xLL)
    {
        handleRelativeAddressingMode(pThis, pAddressingMode, pOpcodeEntry->opcodeRelative);
        return;
    }
    
    handleZeroPageOrAbsoluteAddressingModes(pThis, 
                                            pAddressingMode, 
                                            pOpcodeEntry->opcodeZeroPage, pOpcodeEntry->opcodeAbsolute);
}

static void handleRelativeAddressingMode(Assembler* pThis, AddressingMode* pAddressingMode, unsigned char opcodeRelative)
{
    unsigned short nextInstructionAddress = pThis->pLineInfo->address + 2;
    int            offset = (int)pAddressingMode->expression.value - (int)nextInstructionAddress;
    
    if (!expressionContainsForwardReference(&pAddressingMode->expression) && (offset < -128 || offset > 127))
    {
        LOG_ERROR(pThis, "Relative offset of '%.*s' exceeds the allowed -128 to 127 range.", 
                  pThis->parsedLine.operands.stringLength, pThis->parsedLine.operands.pString);
        return;
    }
    
    emitTwoByteInstruction(pThis, opcodeRelative, (unsigned short)offset);
}

static int expressionContainsForwardReference(Expression* pExpression)
{
    return pExpression->flags & EXPRESSION_FLAG_FORWARD_REFERENCE;
}

static void handleZeroPageOrAbsoluteAddressingModes(Assembler*         pThis, 
                                                    AddressingMode*    pAddressingMode, 
                                                    unsigned char      opcodeZeroPage,
                                                    unsigned char      opcodeAbsolute)
{
    if (pAddressingMode->expression.type == TYPE_ZEROPAGE && opcodeZeroPage != _xXX && opcodeZeroPage != _xLL)
        emitTwoByteInstruction(pThis, opcodeZeroPage, (unsigned short)pAddressingMode->expression.value);
    else if (opcodeAbsolute != _xXX)
    {
        if (opcodeZeroPage == _xLL)
            emitFourByteInstruction(pThis, opcodeAbsolute, pAddressingMode->expression.value);
        else
            emitThreeByteInstruction(pThis, opcodeAbsolute, (unsigned short)pAddressingMode->expression.value);
    }
    else
        logInvalidAddressingModeError(pThis);
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

static void emitFourByteInstruction(Assembler* pThis, unsigned char opCode, uint32_t value)
{
    __try
        allocateLineInfoMachineCodeBytes(pThis, 4);
    __catch
        __nothrow;
    pThis->pLineInfo->pMachineCode[0] = opCode;
    pThis->pLineInfo->pMachineCode[1] = LO_BYTE(value);
    pThis->pLineInfo->pMachineCode[2] = HI_BYTE(value);
    pThis->pLineInfo->pMachineCode[3] = HIHI_BYTE(value);
}

static void handleShortImmediateAddressingMode(Assembler*      pThis, 
                                               AddressingMode* pAddressingMode, 
                                               unsigned char   opcodeImmediate)
{
    handleTwoByteAddressingMode(pThis, pAddressingMode, opcodeImmediate);
}

static void handleLongImmediateAddressingMode(Assembler*      pThis, 
                                              AddressingMode* pAddressingMode, 
                                              unsigned char   opcodeImmediate)
{
    handleThreeByteAddressingMode(pThis, pAddressingMode, opcodeImmediate);
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
    emitTwoByteInstruction(pThis, opcode, (unsigned short)pAddressingMode->expression.value);
}

static void handleThreeByteAddressingMode(Assembler*      pThis, 
                                          AddressingMode* pAddressingMode, 
                                          unsigned char   opcode)
{
    if (opcode == _xXX)
    {
        logInvalidAddressingModeError(pThis);
        return;
    }
    emitTwoByteInstruction(pThis, opcode, (unsigned short)pAddressingMode->expression.value);
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
        Expression  expression;
        
        validateOperandWasProvided(pThis);
        expression = ExpressionEval(pThis, &pThis->parsedLine.operands);
        validateEQULabelFormat(pThis);
        pThis->pLineInfo->flags |= LINEINFO_FLAG_WAS_EQU;
        pThis->pLineInfo->equValue = expression.value;
        attemptToAddSymbol(pThis, &pThis->parsedLine.label, &expression);
    }
    __catch
    {
        __nothrow;
    }
}

static void validateOperandWasProvided(Assembler* pThis)
{
    if (SizedString_strlen(&pThis->parsedLine.operands) > 0)
        return;

    LOG_ERROR(pThis, "%.*s directive requires operand.", 
              pThis->parsedLine.op.stringLength, pThis->parsedLine.op.pString);
    __throw(missingOperandException);
}

static void validateEQULabelFormat(Assembler* pThis)
{
    if (SizedString_strlen(&pThis->parsedLine.label) == 0)
    {
        LOG_ERROR(pThis, "%s directive requires a line label.", "EQU");
        __throw(invalidArgumentException);
    }
    if (isLocalLabelName(&pThis->parsedLine.label))
    {
        LOG_ERROR(pThis, "'%.*s' can't be a local label when used with EQU.", 
                  pThis->parsedLine.label.stringLength, pThis->parsedLine.label.pString);
        __throw(invalidArgumentException);
    }
}

static void updateLinesWhichForwardReferencedThisLabel(Assembler* pThis, Symbol* pSymbol)
{
    LineInfo* pCurr;
    
    if (!pSymbol)
        return;
    if (symbolContainsForwardReferences(pSymbol))
        return;
        
    Symbol_LineReferenceEnumStart(pSymbol);
    while (NULL != (pCurr = Symbol_LineReferenceEnumNext(pSymbol)))
        updateLineWithForwardReference(pThis, pSymbol, pCurr);
}

static int symbolContainsForwardReferences(Symbol* pSymbol)
{
    return expressionContainsForwardReference(&pSymbol->expression);
}

static void updateLineWithForwardReference(Assembler* pThis, Symbol* pSymbol, LineInfo* pLineInfo)
{
    LineInfo*   pLineInfoSave;
    ParsedLine  parsedLineSave;
    
    pLineInfoSave = pThis->pLineInfo;
    parsedLineSave = pThis->parsedLine;
    pThis->pLineInfo = pLineInfo;

    Symbol_LineReferenceRemove(pSymbol, pLineInfo);
    flagLineInfoAsProcessingForwardReference(pLineInfo);
    ParseLine(&pThis->parsedLine, &pLineInfo->lineText);
    firstPassAssembleLine(pThis);

    resetLineInfoAsNotProcessingForwardReference(pLineInfo);
    pThis->parsedLine = parsedLineSave;
    pThis->pLineInfo = pLineInfoSave;
}

static void flagLineInfoAsProcessingForwardReference(LineInfo* pLineInfo)
{
    pLineInfo->flags |= LINEINFO_FLAG_FORWARD_REFERENCE;
}

static void resetLineInfoAsNotProcessingForwardReference(LineInfo* pLineInfo)
{
    pLineInfo->flags &= ~LINEINFO_FLAG_FORWARD_REFERENCE;
}

static void ignoreOperator(Assembler* pThis)
{
}

static void handleInvalidOperator(Assembler* pThis)
{
    LOG_ERROR(pThis, "'%.*s' is not a recognized mnemonic or macro.", 
              pThis->parsedLine.op.stringLength, pThis->parsedLine.op.pString);
}

static void handleASC(Assembler* pThis)
{
    __try
    {
        size_t         i = 0;
        int            alreadyAllocated = isMachineCodeAlreadyAllocatedFromForwardReference(pThis);
        SizedString    operands;
        char           delimiter;
        const char*    pCurr;
        unsigned char  mask;
        unsigned char  byte;

        validateOperandWasProvided(pThis);
        operands = fullOperandStringWithSpaces(pThis);
        SizedString_EnumStart(&operands, &pCurr);
        delimiter = SizedString_EnumNext(&operands, &pCurr);
        mask = delimiter < '\'' ? 0x80 : 0x00;

        while (SizedString_EnumRemaining(&operands, pCurr) && 
               (byte = SizedString_EnumNext(&operands, &pCurr)) != delimiter)
        {
            byte |= mask;
            if (!alreadyAllocated)
                reallocLineInfoMachineCodeBytes(pThis, i+1);
            pThis->pLineInfo->pMachineCode[i] = byte;
            i++;
        }
        assert ( !alreadyAllocated || i == pThis->pLineInfo->machineCodeSize );
    
        if (byte != delimiter)
            LOG_ERROR(pThis, "%.*s didn't end with the expected %c delimiter.", 
                      operands.stringLength, operands.pString,
                      delimiter);
        
        parseHexData(pThis, &operands, &pCurr, alreadyAllocated, i);
    }
    __catch
    {
        reallocLineInfoMachineCodeBytes(pThis, 0);
        __nothrow;
    }
}

static SizedString fullOperandStringWithSpaces(Assembler* pThis)
{
    const char* pStart = pThis->parsedLine.operands.pString;
    const char* pEnd = pThis->pLineInfo->lineText.pString + pThis->pLineInfo->lineText.stringLength;
    
    return SizedString_Init(pStart, pEnd - pStart);
}

static void handleREV(Assembler* pThis)
{
    handleASC(pThis);
    reverseMachineCode(pThis->pLineInfo);
}

static void reverseMachineCode(LineInfo* pLineInfo)
{
    unsigned char* pLower = &pLineInfo->pMachineCode[0];
    unsigned char* pUpper = &pLineInfo->pMachineCode[pLineInfo->machineCodeSize-1];
    
    while (pLower < pUpper)
    {
        char temp = *pLower;
        *pLower++ = *pUpper;
        *pUpper-- = temp;
    }
}

static void handleDUM(Assembler* pThis)
{
    __try
    {
        Expression  expression;
        
        validateOperandWasProvided(pThis);
        expression = getAbsoluteExpression(pThis, &pThis->parsedLine.operands);

        if (!isAlreadyInDUMSection(pThis))
            pThis->programCounterBeforeDUM = pThis->programCounter;
        pThis->programCounter = expression.value;
        pThis->pCurrentBuffer = pThis->pDummyBuffer;
    }
    __catch
    {
        __nothrow;
    }
}

static Expression getAbsoluteExpression(Assembler* pThis, SizedString* pOperands)
{
    Expression expression;
    
    expression = ExpressionEval(pThis, pOperands);
    if (!isTypeAbsolute(&expression))
    {
        LOG_ERROR(pThis, "'%.*s' doesn't specify an absolute address.", pOperands->stringLength, pOperands->pString);
        __throw(invalidArgumentException);
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

static void handleDEND(Assembler* pThis)
{
    __try
    {
        warnIfOperandWasProvided(pThis);
        if (!isAlreadyInDUMSection(pThis))
        {
            LOG_ERROR(pThis, "%.*s isn't allowed without a preceding DUM directive.", 
                      pThis->parsedLine.op.stringLength, pThis->parsedLine.op.pString);
            __throw(invalidArgumentException);
        }

        pThis->programCounter = pThis->programCounterBeforeDUM;
        pThis->pCurrentBuffer = pThis->pObjectBuffer;
    }
    __catch
    {
        __nothrow;
    }
}

static void warnIfOperandWasProvided(Assembler* pThis)
{
    if (SizedString_strlen(&pThis->parsedLine.operands) == 0)
        return;

    LOG_WARNING(pThis, "%.*s directive ignoring operand as comment.", 
                pThis->parsedLine.op.stringLength, pThis->parsedLine.op.pString);
}

static void handleDS(Assembler* pThis)
{
    __try
    {
        SizedString   beforeComma;
        SizedString   afterComma;
        Expression    countExpression;
        Expression    fillExpression;

        validateOperandWasProvided(pThis);
        memset(&fillExpression, 0, sizeof(fillExpression));
        SizedString_SplitString(&pThis->parsedLine.operands, ',', &beforeComma, &afterComma);
        countExpression = getCountExpression(pThis, &beforeComma);
        if (afterComma.stringLength > 0)
            fillExpression = getAbsoluteExpression(pThis, &afterComma);
        saveDSInfoInLineInfo(pThis, countExpression.value, (unsigned char)fillExpression.value);
    }
    __catch
    {
        __nothrow;
    }
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
    __try
    {
        SizedString* pOperands = &pThis->parsedLine.operands;
        int          alreadyAllocated = isMachineCodeAlreadyAllocatedFromForwardReference(pThis);
        const char*  pCurr;
        
        validateOperandWasProvided(pThis);

        SizedString_EnumStart(pOperands, &pCurr);
        parseHexData(pThis, pOperands, &pCurr, alreadyAllocated, 0);
    
        if (pThis->pLineInfo->machineCodeSize > 32)
        {
            reallocLineInfoMachineCodeBytes(pThis, 32);
            LOG_ERROR(pThis, "'%.*s' contains more than 32 values.", 
                      pThis->parsedLine.operands.stringLength, pThis->parsedLine.operands.pString);
        }
    }
    __catch
    {
        if (getExceptionCode() == encounteredCommentException)
            __nothrow;
        logHexParseError(pThis);
        reallocLineInfoMachineCodeBytes(pThis, 0);
        __nothrow;
    }
}

static void parseHexData(Assembler* pThis, const SizedString* pOperands, const char** ppCurr, int alreadyAllocated, size_t i)
{
    __try
    {
        while (SizedString_EnumCurr(pOperands, *ppCurr) != '\0')
        {
            unsigned int   byte;

            byte = getNextHexByte(pOperands, ppCurr);
            if (!alreadyAllocated)
                reallocLineInfoMachineCodeBytes(pThis, i+1);
            pThis->pLineInfo->pMachineCode[i++] = byte;
        }
    }
    __catch
    {
        if (getExceptionCode() != encounteredCommentException)
            __rethrow;
        clearExceptionCode();
    }
    assert ( !alreadyAllocated || i == pThis->pLineInfo->machineCodeSize );
}

static unsigned char getNextHexByte(const SizedString* pString, const char** ppCurr)
{
    unsigned char value = 0;
    
    if (SizedString_EnumCurr(pString, *ppCurr) == ',')
        SizedString_EnumNext(pString, ppCurr);
    
    value = hexCharToNibble(SizedString_EnumNext(pString, ppCurr)) << 4;
    if (SizedString_EnumCurr(pString, *ppCurr) == '\0')
        __throw(invalidArgumentException);
    value |= hexCharToNibble(SizedString_EnumNext(pString, ppCurr));

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
    if (isspace(value))
        __throw(encounteredCommentException);
    __throw(invalidHexDigitException);
}

static void logHexParseError(Assembler* pThis)
{
    if (getExceptionCode() == invalidArgumentException)
        LOG_ERROR(pThis, "'%.*s' doesn't contain an even number of hex digits.", 
                  pThis->parsedLine.operands.stringLength, pThis->parsedLine.operands.pString);
    else if (getExceptionCode() == invalidHexDigitException)
        LOG_ERROR(pThis, "'%.*s' contains an invalid hex digit.",
                  pThis->parsedLine.operands.stringLength, pThis->parsedLine.operands.pString);
}

static void handleORG(Assembler* pThis)
{    
    __try
    {
        Expression  expression;
        
        validateOperandWasProvided(pThis);
        expression = getAbsoluteExpression(pThis, &pThis->parsedLine.operands);
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
        validateOperandWasProvided(pThis);
        BinaryBuffer_QueueWriteToFile(pThis->pObjectBuffer, 
                                      pThis->pInitParams ? pThis->pInitParams->pOutputDirectory : NULL, 
                                      &pThis->parsedLine.operands,
                                      NULL);
    }
    __catch
    {
        if (getExceptionCode() != missingOperandException)
            LOG_ERROR(pThis, "Failed to queue up save to '%.*s'.", 
                      pThis->parsedLine.operands.stringLength, pThis->parsedLine.operands.pString);
        __nothrow;
    }
}

static void handleDB(Assembler* pThis)
{
    __try
    {
        size_t      i = 0;
        SizedString nextOperands;
        int         alreadyAllocated;

        validateOperandWasProvided(pThis);
        nextOperands = pThis->parsedLine.operands;
        alreadyAllocated = isMachineCodeAlreadyAllocatedFromForwardReference(pThis);
        while (SizedString_strlen(&nextOperands) != 0)
        {
            Expression  expression;
            SizedString beforeComma;
            SizedString afterComma;

            SizedString_SplitString(&nextOperands, ',', &beforeComma, &afterComma);
            expression = ExpressionEval(pThis, &beforeComma);
            if (!alreadyAllocated)
                reallocLineInfoMachineCodeBytes(pThis, i + 1);
            pThis->pLineInfo->pMachineCode[i++] = (unsigned char)expression.value;
            nextOperands = afterComma;
        }
        assert ( !alreadyAllocated || i == pThis->pLineInfo->machineCodeSize );
    }
    __catch
    {
        reallocLineInfoMachineCodeBytes(pThis, 0);
        __nothrow;
    }
}

static void handleDA(Assembler* pThis)
{
    __try
    {
        size_t      i = 0;
        SizedString nextOperands;
        int         alreadyAllocated;

        validateOperandWasProvided(pThis);
        nextOperands = pThis->parsedLine.operands;
        alreadyAllocated = isMachineCodeAlreadyAllocatedFromForwardReference(pThis);
        while (SizedString_strlen(&nextOperands) != 0)
        {
            Expression  expression;
            SizedString beforeComma;
            SizedString afterComma;

            SizedString_SplitString(&nextOperands, ',', &beforeComma, &afterComma);
            expression = ExpressionEval(pThis, &beforeComma);
            if (!alreadyAllocated)
                reallocLineInfoMachineCodeBytes(pThis, i+2);
            pThis->pLineInfo->pMachineCode[i++] = (unsigned char)expression.value;
            pThis->pLineInfo->pMachineCode[i++] = (unsigned char)(expression.value >> 8);
            nextOperands = afterComma;
        }
        assert ( !alreadyAllocated || i == pThis->pLineInfo->machineCodeSize );
    }
    __catch
    {
        reallocLineInfoMachineCodeBytes(pThis, 0);
        __nothrow;
    }
}

static void handleXC(Assembler* pThis)
{
    if (0 == SizedString_strcasecmp(&pThis->parsedLine.operands, "OFF"))
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

static void handleMX(Assembler *pThis)
{
    __try
    {
        Expression expression;
        uint32_t value;
        validateOperandWasProvided(pThis);
        expression = getAbsoluteExpression(pThis, &pThis->parsedLine.operands);
        value = expression.value;
        if ((value & ~(uint32_t)3) != 0)
        {
            LOG_ERROR(pThis, "Unknown bits in %s", "MX");
            return;
        }
        if ((value & 2) != 0)
            pThis->longXY = 1;
        else
            pThis->longXY = 0;
        if ((value & 1) != 0)
            pThis->longA = 1;
        else
            pThis->longA = 0;
    }
    __catch
    {
        __nothrow;
    }
}

static void handleREP(Assembler *pThis)
{
    __try
    {
        Expression expression;
        uint32_t value;
        validateOperandWasProvided(pThis);
        expression = getAbsoluteExpression(pThis, &pThis->parsedLine.operands);
        value = expression.value;
        emitTwoByteInstruction(pThis, 0xC2, value);
        if ((value & 0x10) != 0)
            pThis->longXY = 0;
        if ((value & 0x20) != 0)
            pThis->longA = 0;
    }
    __catch
    {
        __nothrow;
    }
}

static void handleSEP(Assembler *pThis)
{
    __try
    {
        Expression expression;
        uint32_t value;
        validateOperandWasProvided(pThis);
        expression = getAbsoluteExpression(pThis, &pThis->parsedLine.operands);
        value = expression.value;
        emitTwoByteInstruction(pThis, 0xE2, value);
        if ((value & 0x10) != 0)
            pThis->longXY = 1;
        if ((value & 0x20) != 0)
            pThis->longA = 1;
    }
    __catch
    {
        __nothrow;
    }
}

static void handleXCE(Assembler *pThis)
{
    __try
    {
        warnIfOperandWasProvided(pThis);
        emitSingleByteInstruction(pThis, 0xFB);
        pThis->longA = pThis->longXY = 0;
    }
    __catch
    {
        __nothrow;
    }
}

static void handleMVx(Assembler* pThis, unsigned char opCode, const char *mnemonic)
{
    uint32_t destinationAddress = 0, sourceAddress = 0;
    __try
    {
        SizedString remainingArguments = pThis->parsedLine.operands;
        validateOperandWasProvided(pThis);
        /* Assume that all operands have known values at this point.  This
           is indeed the case with the Prince of Persia source code.
           -- tkchia 20131012
         */
        destinationAddress = getNextCommaSeparatedLongArgument(pThis, &remainingArguments);
        sourceAddress = getNextCommaSeparatedLongArgument(pThis, &remainingArguments);
        if (SizedString_strlen(&remainingArguments) != 0)
            __throw(invalidArgumentCountException);

    }
    __catch
    {
        if (getExceptionCode() == invalidArgumentCountException || getExceptionCode() == missingOperandException)
            LOG_ERROR(pThis, "Bad number of arguments to %s instruction.", mnemonic);
        __nothrow;
    }
    __try
        allocateLineInfoMachineCodeBytes(pThis, 3);
    __catch
        __nothrow;
    pThis->pLineInfo->pMachineCode[0] = opCode;
    pThis->pLineInfo->pMachineCode[1] = HIHI_BYTE(destinationAddress);
    pThis->pLineInfo->pMachineCode[2] = HIHI_BYTE(sourceAddress);
}

static void handleMVN(Assembler* pThis)
{
    handleMVx(pThis, 0x54, "MVN");
}

static void handleMVP(Assembler* pThis)
{
    handleMVx(pThis, 0x44, "MVP");
}

static void handlePUT(Assembler* pThis)
{
    TextFile*    pIncludedFile = NULL;
    SizedString* pOperands = &pThis->parsedLine.operands;
    SizedString  filename;
    unsigned short numberOfInitialLinesToSkip;

#if 0
    if (isProcessingTextFromPutFile(pThis))
    {
        LOG_ERROR(pThis, "Can't nest PUT directive within another %s file.", "PUT");
        return;
    }
#endif

    __try
    {
        filename = getNextCommaSeparatedStringArgument(pThis, pOperands);
        /* ignore slot number (if present) */
        getNextCommaSeparatedOptionalArgument(pThis, pOperands, 0);
        /* ignore drive number (if present) */
        getNextCommaSeparatedOptionalArgument(pThis, pOperands, 0);
        numberOfInitialLinesToSkip = getNextCommaSeparatedOptionalArgument(pThis, pOperands, 0);
    }
    __catch
    {
        LOG_ERROR(pThis, "Cannot parse %s.", "PUT");
        __nothrow;
    }

    __try
    {
        TextSource* pTextSource = NULL;

        pIncludedFile = openPutFileUsingSearchPath(pThis, &filename);
        pTextSource = TextFileSource_Create(pIncludedFile);
        while (numberOfInitialLinesToSkip != 0) {
            TextSource_GetNextLine(pTextSource);
            --numberOfInitialLinesToSkip;
        }
        TextSource_StackPush(&pThis->pTextSourceStack, pTextSource);
        pIncludedFile = NULL;
    }
    __catch
    {
        TextFile_Free(pIncludedFile);
        LOG_ERROR(pThis, "Failed to PUT '%.*s.S' source file.", filename.stringLength, filename.pString);
        __nothrow;
    }
}

static TextFile* openPutFileUsingSearchPath(Assembler* pThis, const SizedString* pFilename)
{
    TextFile*          pTextFile = NULL;
    size_t             fieldCount;
    const SizedString* pFields;
    size_t             i;
    
    if (!pThis->pPutSearchPath)
        return TextFile_CreateFromFile(NULL, pFilename, ".S");
        
    fieldCount = ParseCSV_FieldCount(pThis->pPutSearchPath);
    pFields = ParseCSV_FieldPointers(pThis->pPutSearchPath);
    for (i = 0 ; i < fieldCount ; i++)
    {
        __try
        {
            pTextFile = TextFile_CreateFromFile(&pFields[i], pFilename, ".S");
            break;
        }
        __catch
        {
            // Failed to open in this directory so clear exception and try again.
            clearExceptionCode();
        }
    }
    
    if (i == fieldCount)
        __throw(fileOpenException);
    return pTextFile;
}

#if 0
static int isProcessingTextFromPutFile(Assembler* pThis)
{
    return TextSource_StackDepth(pThis->pTextSourceStack) > 1;
}
#endif

static void handleUSR(Assembler* pThis)
{
    SizedString fullFilename = SizedString_InitFromString(TextSource_GetFirstFilename(pThis->pTextSourceStack));
    SizedString filename = removeDirectoryAndSuffixFromFullFilename(&fullFilename);

    __try
    {
        SizedString remainingArguments = pThis->parsedLine.operands;
        unsigned short side;
        unsigned short track;
        unsigned short offset;
        
        validateOperandWasProvided(pThis);
        
        /* NOTE: &remainingArguments is an in/out parameter in the following calls to getNextCommaSeparatedArgument()
                 so that it points after the next comma in the argument list after parsing out the previous argument. */
        side = getNextCommaSeparatedArgument(pThis, &remainingArguments);
        track = getNextCommaSeparatedArgument(pThis, &remainingArguments);
        offset = getNextCommaSeparatedArgument(pThis, &remainingArguments);
        /* Last field is length but not required for queueing API as it knows the actual size. */
        getNextCommaSeparatedArgument(pThis, &remainingArguments);
        if (SizedString_strlen(&remainingArguments) != 0)
            __throw(invalidArgumentCountException);

        BinaryBuffer_QueueRW18WriteToFile(pThis->pObjectBuffer, 
                                          pThis->pInitParams ? pThis->pInitParams->pOutputDirectory : NULL, 
                                          &filename,
                                          NULL,
                                          side, track, offset);
    }
    __catch
    {
        if (getExceptionCode() == invalidArgumentCountException)
            LOG_ERROR(pThis, "'%.*s' doesn't contain the 4 arguments required for USR directive.",
                      pThis->parsedLine.operands.stringLength, pThis->parsedLine.operands.pString);
        else if (getExceptionCode() != missingOperandException)
            LOG_ERROR(pThis, "Failed to queue up USR save to '%.*s'.", 
                      filename.stringLength, filename.pString);
        __nothrow;
    }
}

static SizedString removeDirectoryAndSuffixFromFullFilename(SizedString* pFullFilename)
{
    const char* pLastDot = SizedString_strrchr(pFullFilename, '.');
    const char* pLastSlash = SizedString_strrchr(pFullFilename, PATH_SEPARATOR);
    const char* pFullFilenameEnd = pFullFilename->pString + pFullFilename->stringLength;
    const char* pStart = pLastSlash ? pLastSlash + 1 : pFullFilename->pString;
    int         length = pLastDot > pLastSlash ? pLastDot - pStart : pFullFilenameEnd - pStart;
    SizedString result = SizedString_Init(pStart, length);
    
    return result;
}

static SizedString getNextCommaSeparatedOptionalStringArgument(Assembler* pThis, SizedString* pRemainingArguments)
{
    SizedString beforeComma;
    SizedString afterComma;
    
    SizedString_SplitString(pRemainingArguments, ',', &beforeComma, &afterComma);
    *pRemainingArguments = afterComma;
    return beforeComma;
}

static SizedString getNextCommaSeparatedStringArgument(Assembler* pThis, SizedString* pRemainingArguments)
{
    SizedString beforeComma = getNextCommaSeparatedOptionalStringArgument(pThis, pRemainingArguments);
    if (SizedString_strlen(&beforeComma) == 0)
        __throw(invalidArgumentCountException);
    return beforeComma;
}

static uint32_t getNextCommaSeparatedLongArgument(Assembler* pThis, SizedString* pRemainingArguments)
{
    SizedString beforeComma = getNextCommaSeparatedStringArgument(pThis, pRemainingArguments);
    Expression expression;
    expression = ExpressionEval(pThis, &beforeComma);
    return expression.value;
}

static unsigned short getNextCommaSeparatedArgument(Assembler* pThis, SizedString* pRemainingArguments)
{
    return (unsigned short)getNextCommaSeparatedLongArgument(pThis, pRemainingArguments);
}

static unsigned short getNextCommaSeparatedOptionalArgument(Assembler* pThis, SizedString* pRemainingArguments, unsigned short defaultValue)
{
    SizedString beforeComma = getNextCommaSeparatedOptionalStringArgument(pThis, pRemainingArguments);
    if (SizedString_strlen(&beforeComma) == 0)
        return defaultValue;
    Expression expression = ExpressionEval(pThis, &beforeComma);
    return expression.value;
}

static void handleDO(Assembler* pThis)
{
    if (isUpdatingForwardReference(pThis))
        return;
        
    __try
    {
        Expression  expression;

        validateOperandWasProvided(pThis);
        disallowForwardReferences(pThis);
        expression = ExpressionEval(pThis, &pThis->parsedLine.operands);
        pushConditional(pThis, doesExpressionEqualZeroIndicatingToSkipSourceLines(&expression));
    }
    __catch
    {
        __nothrow;
    }
}

static int isUpdatingForwardReference(Assembler* pThis)
{
    return pThis->pLineInfo->flags & LINEINFO_FLAG_FORWARD_REFERENCE;
}

static void disallowForwardReferences(Assembler* pThis)
{
    pThis->pLineInfo->flags |= LINEINFO_FLAG_DISALLOW_FORWARD;
}

static int doesExpressionEqualZeroIndicatingToSkipSourceLines(Expression* pExpression)
{
    return pExpression->value == 0;
}

static void pushConditional(Assembler* pThis, int skipSourceLines)
{
    __try
    {
        Conditional* pAlloc = allocateAndZero(sizeof(*pAlloc));
        if (skipSourceLines)
            pAlloc->flags |= CONDITIONAL_SKIP_SOURCE;
        pAlloc->flags |= determineInheritedConditionalSkipSourceLineState(pThis);
        pAlloc->pPrev = pThis->pConditionals;
        pAlloc->pLineInfo = pThis->pLineInfo;
        pThis->pConditionals = pAlloc;
    }
    __catch
    {
        LOG_ERROR(pThis, "Failed to allocate space for %s conditional storage.", "DO");
        __rethrow;
    }
}

static unsigned int determineInheritedConditionalSkipSourceLineState(Assembler* pThis)
{
    unsigned int parentSkipState = pThis->pConditionals ? 
                                        pThis->pConditionals->flags & CONDITIONAL_SKIP_STATES_MASK : 
                                        0;
    
    return parentSkipState ? CONDITIONAL_INHERITED_SKIP_SOURCE : 0;
}

static void handleELSE(Assembler* pThis)
{
    __try
    {
        warnIfOperandWasProvided(pThis);
        flipTopConditionalState(pThis);
    }
    __catch
    {
        __nothrow;
    }
}

static void flipTopConditionalState(Assembler* pThis)
{
    validateInConditionalAlready(pThis);
    flipConditionalState(pThis->pConditionals);
    validateElseNotAlreadySeen(pThis);
    rememberThatConditionalHasSeenElse(pThis->pConditionals);
}

static void validateInConditionalAlready(Assembler* pThis)
{
    if (pThis->pConditionals)
        return;

    LOG_WARNING(pThis, "%.*s directive without corresponding DO/IF directive.",
              pThis->parsedLine.op.stringLength, pThis->parsedLine.op.pString);
    __throw(invalidArgumentException);
}

static void flipConditionalState(Conditional* pConditional)
{
    pConditional->flags ^= CONDITIONAL_SKIP_SOURCE;
}

static void validateElseNotAlreadySeen(Assembler* pThis)
{
    if (!seenElseAlready(pThis->pConditionals))
        return;
        
    LOG_ERROR(pThis, "Can't have multiple %s directives in a DO/IF clause.", "ELSE");
    __throw(invalidArgumentException);
}

static int seenElseAlready(Conditional* pConditional)
{
    return (int)(pConditional->flags & CONDITIONAL_SEEN_ELSE);
}

static void rememberThatConditionalHasSeenElse(Conditional* pConditional)
{
    pConditional->flags |= CONDITIONAL_SEEN_ELSE;
}

static void handleFIN(Assembler* pThis)
{
    __try
    {
        warnIfOperandWasProvided(pThis);
        popConditional(pThis);
    }
    __catch
    {
        __nothrow;
    }
}

static void popConditional(Assembler* pThis)
{
    Conditional* pPrev;
    
    validateInConditionalAlready(pThis);
    pPrev = pThis->pConditionals->pPrev;
    free(pThis->pConditionals);
    pThis->pConditionals = pPrev;
}

static void handleLUP(Assembler* pThis)
{
    TextFile*   pLoopTextFile = NULL;
    TextSource* pTextSource = NULL;
    TextFile*   pBaseTextFile = TextSource_GetTextFile(pThis->pTextSourceStack);

    __try
    {
        Expression     expression;
        SizedString    nextLine;
        ParsedLine     parsedLine;

        disallowForwardReferences(pThis);
        flagThatLupDirectiveHasBeenSeen(pThis);
        validateOperandWasProvided(pThis);
        expression = ExpressionEval(pThis, &pThis->parsedLine.operands);
        validateLupExpressionInRange(pThis, &expression);
        
        pLoopTextFile = TextFile_CreateFromTextFile(pBaseTextFile);
        while (!TextFile_IsEndOfFile(pLoopTextFile))
        {
            nextLine = TextFile_GetNextLine(pLoopTextFile);
            ParseLine(&parsedLine, &nextLine);
            if (0 == SizedString_strcmp(&parsedLine.op, "--^"))
                break;
        }

        validateThatLupEndWasFound(pThis, &parsedLine);
        TextFile_SetEndOfFile(pLoopTextFile);
        TextFile_AdvanceTo(pBaseTextFile, pLoopTextFile);

        TextFile_Reset(pLoopTextFile);
        pTextSource = LupSource_Create(pLoopTextFile, expression.value);
        pLoopTextFile = NULL;
        TextSource_StackPush(&pThis->pTextSourceStack, pTextSource);
    }
    __catch
    {
        TextFile_Free(pLoopTextFile);
        if (getExceptionCode() == outOfMemoryException)
            LOG_ERROR(pThis, "Failed to allocate memory for %s directive.", "LUP");
        __nothrow;
    }
}

static void flagThatLupDirectiveHasBeenSeen(Assembler* pThis)
{
    pThis->seenLUP = 1;
}

static void validateLupExpressionInRange(Assembler* pThis, Expression* pExpression)
{
    unsigned short value = pExpression->value;
    if (value >= 1 && value <= 0x8000)
        return;
        
    LOG_WARNING(pThis, "LUP directive count of %u doesn't fall in valid range of 1 to 32768.", value);
    __throw(invalidArgumentException);
}

static void validateThatLupEndWasFound(Assembler* pThis, ParsedLine* pParsedLine)
{
    if (0 == SizedString_strcmp(&pParsedLine->op, "--^"))
        return;
    
    LOG_ERROR(pThis, "%s directive is missing matching --^ directive.", "LUP");
    __throw(invalidArgumentException);
}

static void handleLUPend(Assembler* pThis)
{
    if (!haveSeenLupDirective(pThis))
    {
        LOG_ERROR(pThis, "%s directive without corresponding LUP directive.", "--^");
        return;
    }
    
    __try
    {
        clearLupDirectiveFlag(pThis);
        warnIfOperandWasProvided(pThis);
    }
    __catch
    {
        __nothrow;
    }
}

static int haveSeenLupDirective(Assembler* pThis)
{
    return pThis->seenLUP;
}

static void clearLupDirectiveFlag(Assembler* pThis)
{
    pThis->seenLUP = 0;
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
        LOG_ERROR(pThis, "The '%.*s%.*s' label is undefined.", 
                  pSymbol->globalKey.stringLength, pSymbol->globalKey.pString,
                  pSymbol->localKey.stringLength, pSymbol->localKey.pString);
    }
}

static void checkForOpenConditionals(Assembler* pThis)
{
    if (pThis->pConditionals)
        LOG_LINE_WARNING(pThis, pThis->pConditionals->pLineInfo, "%s directive is missing matching FIN directive.", "DO/IF");
}

static void secondPass(Assembler* pThis)
{
    outputListFile(pThis);
    if (pThis->errorCount > 0)
        return;
    __try
    {
        BinaryBuffer_ProcessWriteFileQueue(pThis->pObjectBuffer);
    }
    __catch
    {
        LOG_ERROR(pThis, "Failed to save %s.", "output");
        __rethrow;
    }
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


unsigned int Assembler_GetWarningCount(Assembler* pThis)
{
    return pThis->warningCount;
}


static void throwIfForwardReferencesAreDisallowed(Assembler* pThis);
static int areForwardReferencesDisallowed(Assembler* pThis);
__throws Symbol* Assembler_FindLabel(Assembler* pThis, SizedString* pLabelName)
{
    Symbol*     pSymbol = NULL;
    
    SizedString globalLabel = initGlobalLabelString(pThis, pLabelName);
    SizedString localLabel = initLocalLabelString(pLabelName);

    validateLabelFormat(pThis, pLabelName);
    pSymbol = SymbolTable_Find(pThis->pSymbols, &globalLabel, &localLabel);
    if (!pSymbol)
    {
        throwIfForwardReferencesAreDisallowed(pThis);
        pSymbol = SymbolTable_Add(pThis->pSymbols, &globalLabel, &localLabel);
    }
    if (!isSymbolAlreadyDefined(pSymbol, NULL))
        Symbol_LineReferenceAdd(pSymbol, pThis->pLineInfo);

    return pSymbol;
}

static void throwIfForwardReferencesAreDisallowed(Assembler* pThis)
{
    if (!areForwardReferencesDisallowed(pThis))
        return;

    LOG_ERROR(pThis, "%.*s directive can't forward reference labels.", 
              pThis->parsedLine.op.stringLength, pThis->parsedLine.op.pString);
    __throw(invalidArgumentException);
}

static int areForwardReferencesDisallowed(Assembler* pThis)
{
    return pThis->pLineInfo->flags & LINEINFO_FLAG_DISALLOW_FORWARD;
}
