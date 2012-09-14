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


static void commonObjectInit(Assembler* pThis, const AssemblerInitParams* pParams);
static FILE* createListFileOrRedirectToStdOut(Assembler* pThis, const AssemblerInitParams* pParams);
static void createParseObjectForPutSearchPath(Assembler* ptThis, const AssemblerInitParams* pParams);
static void createFullInstructionSetTables(Assembler* pThis); 
static void create6502InstructionSetTable(Assembler* pThis);
static int compareInstructionSetEntries(const void* pv1, const void* pv2);
static void create65c02InstructionSetTable(Assembler* pThis);
static size_t countNewInstructionMnemonics(const OpCodeEntry* pBaseSet, size_t baseSetLength, 
                                           const OpCodeEntry* pNewSet,  size_t newSetLength);
static int compareInstructionSetEntryToOperatorString(const void* pvKey, const void* pvEntry);
static void updateExistingInstructions(OpCodeEntry* pBaseSet, size_t baseSetLength, size_t totalLength,
                                       const OpCodeEntry* pAddSet,  size_t addSetLength);
static void updateInstructionEntry(OpCodeEntry* pEntryToUpdate, const OpCodeEntry* pAdditionalEntry);
static void setOrgInAssemblerAndBinaryBufferModules(Assembler* pThis, unsigned short orgAddress);
__throws Assembler* Assembler_CreateFromString(char* pText, const AssemblerInitParams* pParams)
{
    Assembler* pThis = NULL;
    
    __try
    {
        pThis = allocateAndZero(sizeof(*pThis));
        pThis->pMainFile = TextFile_CreateFromString(pText);
        commonObjectInit(pThis, pParams);
    }
    __catch
    {
        Assembler_Free(pThis);
        __rethrow;
    }

    return pThis;
}

static void commonObjectInit(Assembler* pThis, const AssemblerInitParams* pParams)
{
    __try
    {
        FILE* pListFile = createListFileOrRedirectToStdOut(pThis, pParams);
        pThis->pListFile = ListFile_Create(pListFile);
        pThis->pSymbols = SymbolTable_Create(NUMBER_OF_SYMBOL_TABLE_HASH_BUCKETS);
        pThis->pObjectBuffer = BinaryBuffer_Create(SIZE_OF_OBJECT_AND_DUMMY_BUFFERS);
        pThis->pDummyBuffer = BinaryBuffer_Create(SIZE_OF_OBJECT_AND_DUMMY_BUFFERS);
        createParseObjectForPutSearchPath(pThis, pParams);
        createFullInstructionSetTables(pThis);
        pThis->pInitParams = pParams;
        pThis->pTextFile = pThis->pMainFile;
        pThis->linesHead.pTextFile = pThis->pMainFile;
        pThis->pLineInfo = &pThis->linesHead;
        pThis->pCurrentBuffer = pThis->pObjectBuffer;
        setOrgInAssemblerAndBinaryBufferModules(pThis, 0x8000);
    }
    __catch
    {
        __rethrow;
    }
}

static FILE* createListFileOrRedirectToStdOut(Assembler* pThis, const AssemblerInitParams* pParams)
{
    if (!pParams || !pParams->pListFilename)
        return stdout;
        
    pThis->pFileForListing = fopen(pParams->pListFilename, "w");
    if (!pThis->pFileForListing)
        __throw(fileNotFoundException);
    return pThis->pFileForListing;
}

static void createParseObjectForPutSearchPath(Assembler* pThis, const AssemblerInitParams* pParams)
{
    ParseCSV* pParser = NULL;
    char*     pPutDirectories = NULL;
    
    if (!pParams || !pParams->pPutDirectories)
        return;
    
    __try
    {
        pPutDirectories = copyOfString(pParams->pPutDirectories);
        pParser = ParseCSV_CreateWithCustomSeparator(';');
        ParseCSV_Parse(pParser, pPutDirectories);
    }
    __catch
    {
        free(pPutDirectories);
        ParseCSV_Free(pParser);
        __rethrow;
    }
    
    pThis->pPutDirectories = pPutDirectories;
    pThis->pPutSearchPath = pParser;
}

static void createFullInstructionSetTables(Assembler* pThis)
{
    __try
    {
        create6502InstructionSetTable(pThis);
        create65c02InstructionSetTable(pThis);

        pThis->instructionSets[2] = allocateAndZero(sizeof(g_6502InstructionSet));
        pThis->instructionSetSizes[2] = ARRAYSIZE(g_6502InstructionSet);
        memcpy(pThis->instructionSets[2], pThis->instructionSets[0], sizeof(g_6502InstructionSet));
    }
    __catch
    {
        __rethrow;
    }
}

static void create6502InstructionSetTable(Assembler* pThis)
{
    __try
    {
        pThis->instructionSets[0] = allocateAndZero(sizeof(g_6502InstructionSet));
        memcpy(pThis->instructionSets[0], g_6502InstructionSet, sizeof(g_6502InstructionSet));
        pThis->instructionSetSizes[0] = ARRAYSIZE(g_6502InstructionSet);
        qsort(pThis->instructionSets[0], ARRAYSIZE(g_6502InstructionSet), sizeof(g_6502InstructionSet[0]), 
              compareInstructionSetEntries);
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

static void create65c02InstructionSetTable(Assembler* pThis)
{
    size_t baseSetLength = pThis->instructionSetSizes[0];
    size_t instructionsAdded = countNewInstructionMnemonics(
                                    pThis->instructionSets[0], baseSetLength, 
                                    g_65c02AdditionalInstructions, ARRAYSIZE(g_65c02AdditionalInstructions));
    size_t newSetLength = baseSetLength + instructionsAdded;
    
    __try
    {
        pThis->instructionSets[1] = allocateAndZero(sizeof(OpCodeEntry) * newSetLength);
        memcpy(pThis->instructionSets[1], pThis->instructionSets[0], sizeof(OpCodeEntry) * baseSetLength);
        updateExistingInstructions(pThis->instructionSets[1], baseSetLength, newSetLength, 
                                   g_65c02AdditionalInstructions, ARRAYSIZE(g_65c02AdditionalInstructions));
        pThis->instructionSetSizes[1] = newSetLength;
        qsort(pThis->instructionSets[1], newSetLength, sizeof(g_6502InstructionSet[0]), compareInstructionSetEntries);
    }
    __catch
    {
        __rethrow;
    }
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

static void updateExistingInstructions(OpCodeEntry* pBaseSet, size_t baseSetLength, size_t totalLength,
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
        SizedString sourceFilename = SizedString_InitFromString(pSourceFilename);
        pThis = allocateAndZero(sizeof(*pThis));
        pThis->pMainFile = TextFile_CreateFromFile(NULL, &sourceFilename, NULL);
        commonObjectInit(pThis, pParams);
    }
    __catch
    {
        Assembler_Free(pThis);
        __rethrow;
    }

    return pThis;
}


static void freeLines(Assembler* pThis);
static void freeInstructionSets(Assembler* pThis);
static void freeIncludedTextFiles(Assembler* pThis);
void Assembler_Free(Assembler* pThis)
{
    if (!pThis)
        return;
    
    freeLines(pThis);
    freeInstructionSets(pThis);
    freeIncludedTextFiles(pThis);
    ParseCSV_Free(pThis->pPutSearchPath);
    free(pThis->pPutDirectories);
    ListFile_Free(pThis->pListFile);
    BinaryBuffer_Free(pThis->pDummyBuffer);
    BinaryBuffer_Free(pThis->pObjectBuffer);
    SymbolTable_Free(pThis->pSymbols);
    TextFile_Free(pThis->pMainFile);
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

static void freeInstructionSets(Assembler* pThis)
{
    size_t i;
    
    for (i = 0 ; i < ARRAYSIZE(pThis->instructionSets) ; i++)
    {
        free(pThis->instructionSets[i]);
    }
}

static void freeIncludedTextFiles(Assembler* pThis)
{
    TextFileNode* pCurr = pThis->pIncludedFiles;
    
    while (pCurr)
    {
        TextFileNode* pNext = pCurr->pNext;
        TextFile_Free(pCurr->pTextFile);
        free(pCurr);
        pCurr = pNext;
    }
}

static void firstPass(Assembler* pThis);
static char* getNextSourceLine(Assembler* pThis);
static int isProcessingTextFromPutFile(Assembler* pThis);
static void parseLine(Assembler* pThis, char* pLine);
static void prepareLineInfoForThisLine(Assembler* pThis, char* pLine);
static void rememberLabel(Assembler* pThis);
static int doesLineContainALabel(Assembler* pThis);
static void validateLabelFormat(Assembler* pThis, SizedString* pLabel);
static int isGlobalLabelName(SizedString* pLabelName);
static int isLocalLabelName(SizedString* pLabelName);
static int seenGlobalLabel(Assembler* pThis);
static Symbol* attemptToAddSymbol(Assembler* pThis, SizedString* pLabelName, Expression* pExpression);
static SizedString initLocalLabelString(SizedString* pLabelName);
static int isSymbolAlreadyDefined(Symbol* pSymbol, LineInfo* pThisLine);
static int isVariableLabelName(SizedString* pLabelName);
static void flagSymbolAsDefined(Symbol* pSymbol, LineInfo* pThisLine);
static void rememberGlobalLabel(Assembler* pThis);
static void firstPassAssembleLine(Assembler* pThis);
static int compareInstructionSetEntryToOperatorSizedString(const void* pvKey, const void* pvEntry);
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
static void validateOperandWasProvided(Assembler* pThis);
static void validateEQULabelFormat(Assembler* pThis);
static void updateLinesWhichForwardReferencedThisLabel(Assembler* pThis, Symbol* pSymbol);
static int symbolContainsForwardReferences(Symbol* pSymbol);
static void updateLineWithForwardReference(Assembler* pThis, Symbol* pSymbol, LineInfo* pLineInfo);
static void handleInvalidOperator(Assembler* pThis);
static const char* fullOperandStringWithSpaces(Assembler* pThis);
static void validateNoOperandWasProvided(Assembler* pThis);
static Expression getAbsoluteExpression(Assembler* pThis, SizedString* pOperands);
static int isTypeAbsolute(Expression* pExpression);
static int isAlreadyInDUMSection(Assembler* pThis);
static Expression getCountExpression(Assembler* pThis, SizedString* pString);
static Expression getBytesLeftInPage(Assembler* pThis);
static void saveDSInfoInLineInfo(Assembler* pThis, unsigned short repeatCount, unsigned char fillValue);
static unsigned char getNextHexByte(SizedString* pString, const char** ppCurr);
static unsigned char hexCharToNibble(char value);
static void logHexParseError(Assembler* pThis);
static void rememberIncludedTextFile(Assembler* pThis, TextFile* pTextFile);
static TextFile* openPutFileUsingSearchPath(Assembler* pThis, const SizedString* pFilename);
static unsigned short getNextCommaSeparatedArgument(Assembler* pThis, SizedString* pRemainingArguments);
static SizedString removeDirectoryAndSuffixFromFullFilename(SizedString* pFullFilename);
static void checkForUndefinedSymbols(Assembler* pThis);
static void checkSymbolForOutstandingForwardReferences(Assembler* pThis, Symbol* pSymbol);
static void secondPass(Assembler* pThis);
static void outputListFile(Assembler* pThis);
void Assembler_Run(Assembler* pThis)
{
    __try
    {
        firstPass(pThis);
        checkForUndefinedSymbols(pThis);
        secondPass(pThis);
    }
    __catch
    {
        __rethrow;
    }
}

static void firstPass(Assembler* pThis)
{
    char*      pLine = NULL;
    
    while (NULL != (pLine = getNextSourceLine(pThis)))
    {
        __try
            parseLine(pThis, pLine);
        __catch
            __rethrow;
    }
}

static char* getNextSourceLine(Assembler* pThis)
{
    char* pLine = TextFile_GetNextLine(pThis->pTextFile);
    
    if (!pLine && isProcessingTextFromPutFile(pThis))
    {
        pThis->pTextFile = pThis->pMainFile;
        pThis->indentation = 0;
        return TextFile_GetNextLine(pThis->pTextFile);
    }

    return pLine;
}

static int isProcessingTextFromPutFile(Assembler* pThis)
{
    return pThis->pTextFile != pThis->pMainFile;
}

static void parseLine(Assembler* pThis, char* pLine)
{
    __try
    {
        prepareLineInfoForThisLine(pThis, pLine);
        ParseLine(&pThis->parsedLine, pLine);
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
    pLineInfo->pTextFile = pThis->pTextFile;
    pLineInfo->lineNumber = TextFile_GetLineNumber(pThis->pTextFile);
    pLineInfo->pLineText = pLine;
    pLineInfo->address = pThis->programCounter;
    pLineInfo->instructionSet = pThis->instructionSet;
    pLineInfo->indentation = pThis->indentation;
    pThis->pLineInfo->pNext = pLineInfo;
    pThis->pLineInfo = pLineInfo;
}

static void rememberLabel(Assembler* pThis)
{
    if (!doesLineContainALabel(pThis))
        return;

    __try
    {
        Symbol*     pSymbol = NULL;
        Expression  expression;
    
        validateLabelFormat(pThis, &pThis->parsedLine.label);
        expression = ExpressionEval_CreateAbsoluteExpression(pThis->programCounter);
        rememberGlobalLabel(pThis);
        pSymbol = attemptToAddSymbol(pThis, &pThis->parsedLine.label, &expression);
    }
    __catch
    {
        __nothrow;
    }
}

static int doesLineContainALabel(Assembler* pThis)
{
    return SizedString_strlen(&pThis->parsedLine.label) > 0;
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

static int isGlobalLabelName(SizedString* pLabelName)
{
    return !isLocalLabelName(pLabelName) && !isVariableLabelName(pLabelName);
}

static int isLocalLabelName(SizedString* pLabelName)
{
    return pLabelName->pString && pLabelName->pString[0] == ':';
}

static int seenGlobalLabel(Assembler* pThis)
{
    return SizedString_strlen(&pThis->globalLabel) > 0;
}

static Symbol* attemptToAddSymbol(Assembler* pThis, SizedString* pLabelName, Expression* pExpression)
{
    SizedString localLabel = initLocalLabelString(pLabelName);
    
    Symbol* pSymbol = SymbolTable_Find(pThis->pSymbols, &pThis->globalLabel, &localLabel);
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
            pSymbol = SymbolTable_Add(pThis->pSymbols, &pThis->globalLabel, &localLabel);
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

    return pSymbol;
}

static SizedString initLocalLabelString(SizedString* pLabelName)
{
    if (isLocalLabelName(pLabelName))
        return *pLabelName;
    return SizedString_InitFromString(NULL);
}

static int isSymbolAlreadyDefined(Symbol* pSymbol, LineInfo* pThisLine)
{
    return pSymbol->pDefinedLine != NULL && pSymbol->pDefinedLine != pThisLine;
}

static int isVariableLabelName(SizedString* pLabelName)
{
    return pLabelName->pString && pLabelName->pString[0] == ']';
}

static void flagSymbolAsDefined(Symbol* pSymbol, LineInfo* pThisLine)
{
    pSymbol->pDefinedLine = pThisLine;
    pThisLine->pSymbol = pSymbol;
}

static void rememberGlobalLabel(Assembler* pThis)
{
    if (!isGlobalLabelName(&pThis->parsedLine.label))
        return;
        
    pThis->globalLabel = pThis->parsedLine.label;
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
        addressingMode = AddressingMode_Eval(pThis, &pThis->parsedLine.operands);
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
        Symbol*     pSymbol = pThis->pLineInfo->pSymbol;
        Expression  expression;
        
        validateOperandWasProvided(pThis);
        expression = ExpressionEval(pThis, &pThis->parsedLine.operands);
        validateEQULabelFormat(pThis);
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
    if (pThis->parsedLine.label.pString[0] == ':')
    {
        LOG_ERROR(pThis, "'%.*s' can't be a local label when used with EQU.", 
                  pThis->parsedLine.label.stringLength, pThis->parsedLine.label.pString);
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
        updateLineWithForwardReference(pThis, pSymbol, pCurr);
}

static int symbolContainsForwardReferences(Symbol* pSymbol)
{
    return pSymbol->expression.flags & EXPRESSION_FLAG_FORWARD_REFERENCE;
}

static void updateLineWithForwardReference(Assembler* pThis, Symbol* pSymbol, LineInfo* pLineInfo)
{
    LineInfo*   pLineInfoSave;
    ParsedLine  parsedLineSave;
    
    pLineInfoSave = pThis->pLineInfo;
    parsedLineSave = pThis->parsedLine;
    pThis->pLineInfo = pLineInfo;

    Symbol_LineReferenceRemove(pSymbol, pLineInfo);
    ParseLine(&pThis->parsedLine, pLineInfo->pLineText);
    firstPassAssembleLine(pThis);

    pThis->parsedLine = parsedLineSave;
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

    LOG_ERROR(pThis, "'%.*s' is not a recognized mnemonic or macro.", 
              pThis->parsedLine.op.stringLength, pThis->parsedLine.op.pString);
}

static void handleASC(Assembler* pThis)
{
    __try
    {
        size_t         i = 0;
        int            alreadyAllocated = isMachineCodeAlreadyAllocatedFromForwardReference(pThis);
        const char*    pOperands;
        char           delimiter;
        const char*    pCurr;
        unsigned char  mask;

        validateOperandWasProvided(pThis);
        pOperands = fullOperandStringWithSpaces(pThis);
        delimiter = *pOperands;
        pCurr = &pOperands[1];
        mask = delimiter < '\'' ? 0x80 : 0x00;

        while (*pCurr && *pCurr != delimiter)
        {
            unsigned char   byte = *pCurr | mask;
            if (!alreadyAllocated)
                reallocLineInfoMachineCodeBytes(pThis, i+1);
            pThis->pLineInfo->pMachineCode[i++] = byte;
            pCurr++;
        }
        assert ( !alreadyAllocated || i == pThis->pLineInfo->machineCodeSize );
    
        if (*pCurr == '\0')
            LOG_ERROR(pThis, "%.*s didn't end with the expected %c delimiter.", 
                      pThis->parsedLine.operands.stringLength, pThis->parsedLine.operands.pString,
                      delimiter);
    }
    __catch
    {
        reallocLineInfoMachineCodeBytes(pThis, 0);
        __nothrow;
    }
}

static const char* fullOperandStringWithSpaces(Assembler* pThis)
{
    return pThis->parsedLine.operands.pString;
}

static void handleDEND(Assembler* pThis)
{
    __try
    {
        validateNoOperandWasProvided(pThis);
        if (!isAlreadyInDUMSection(pThis))
        {
            LOG_ERROR(pThis, "%.*s isn't allowed without a preceding DUM directive.", 
                      pThis->parsedLine.op.stringLength, pThis->parsedLine.op.pString);
            return;
        }

        pThis->programCounter = pThis->programCounterBeforeDUM;
        pThis->pCurrentBuffer = pThis->pObjectBuffer;
    }
    __catch
    {
        __nothrow;
    }
}

static void validateNoOperandWasProvided(Assembler* pThis)
{
    if (SizedString_strlen(&pThis->parsedLine.operands) == 0)
        return;

    LOG_ERROR(pThis, "%.*s directive doesn't require operand.", 
              pThis->parsedLine.op.stringLength, pThis->parsedLine.op.pString);
    __throw(invalidArgumentException);
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
    
    __try
    {
        expression = ExpressionEval(pThis, pOperands);
        if (!isTypeAbsolute(&expression))
        {
            LOG_ERROR(pThis, "'%.*s' doesn't specify an absolute address.", pOperands->stringLength, pOperands->pString);
            __throw(invalidArgumentException);
        }
    }
    __catch
    {
        __rethrow;
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
        size_t       i = 0;
        int          alreadyAllocated = isMachineCodeAlreadyAllocatedFromForwardReference(pThis);
        const char*  pCurr;
        
        validateOperandWasProvided(pThis);

        SizedString_EnumStart(pOperands, &pCurr);
        while (SizedString_EnumCurr(pOperands, pCurr) != '\0' && i < 32)
        {
            unsigned int   byte;

            byte = getNextHexByte(pOperands, &pCurr);
            if (!alreadyAllocated)
                reallocLineInfoMachineCodeBytes(pThis, i+1);
            pThis->pLineInfo->pMachineCode[i++] = byte;
        }
        assert ( !alreadyAllocated || i == pThis->pLineInfo->machineCodeSize );
    
        if (SizedString_EnumCurr(pOperands, pCurr) != '\0')
        {
            LOG_ERROR(pThis, "'%.*s' contains more than 32 values.", 
                      pThis->parsedLine.operands.stringLength, pThis->parsedLine.operands.pString);
            return;
        }
    }
    __catch
    {
        logHexParseError(pThis);
        reallocLineInfoMachineCodeBytes(pThis, 0);
        __nothrow;
    }
}

static unsigned char getNextHexByte(SizedString* pString, const char** ppCurr)
{
    unsigned char value;
    
    __try
    {
        value = hexCharToNibble(SizedString_EnumNext(pString, ppCurr)) << 4;
        if (SizedString_EnumCurr(pString, *ppCurr) == '\0')
            __throw(invalidArgumentException);
        value |= hexCharToNibble(SizedString_EnumNext(pString, ppCurr));

        if (SizedString_EnumCurr(pString, *ppCurr) == ',')
            SizedString_EnumNext(pString, ppCurr);
    }
    __catch
    {
        __rethrow;
    }

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

static void handlePUT(Assembler* pThis)
{
    TextFile*    pIncludedFile = NULL;
    SizedString* pOperands = &pThis->parsedLine.operands;
    
    if (isProcessingTextFromPutFile(pThis))
    {
        LOG_ERROR(pThis, "Can't nest PUT directive within another %s file.", "PUT");
        return;
    }
    
    __try
    {
        validateOperandWasProvided(pThis);
        pIncludedFile = openPutFileUsingSearchPath(pThis, pOperands);
        rememberIncludedTextFile(pThis, pIncludedFile);
        pThis->pTextFile = pIncludedFile;
        pIncludedFile = NULL;
        pThis->indentation = 4;
    }
    __catch
    {
        TextFile_Free(pIncludedFile);
        LOG_ERROR(pThis, "Failed to PUT '%.*s.S' source file.", pOperands->stringLength, pOperands->pString);
        __nothrow;
    }
}

static TextFile* openPutFileUsingSearchPath(Assembler* pThis, const SizedString* pFilename)
{
    size_t       fieldCount;
    const char** ppFields;
    size_t       i;
    
    if (!pThis->pPutSearchPath)
        return TextFile_CreateFromFile(NULL, pFilename, ".S");
        
    fieldCount = ParseCSV_FieldCount(pThis->pPutSearchPath);
    ppFields = ParseCSV_FieldPointers(pThis->pPutSearchPath);
    for (i = 0 ; i < fieldCount ; i++)
    {
        __try
        {
            TextFile* pTextFile = TextFile_CreateFromFile(ppFields[i], pFilename, ".S");
            return pTextFile;
        }
        __catch
        {
            // Failed to open in this directory so clear exception and try again.
            clearExceptionCode();
        }
    }
    
    __throw(fileNotFoundException);
}

static void rememberIncludedTextFile(Assembler* pThis, TextFile* pTextFile)
{
    __try
    {
        TextFileNode* pNode = allocateAndZero(sizeof(*pNode));
        pNode->pNext = pThis->pIncludedFiles;
        pNode->pTextFile = pTextFile;
        pThis->pIncludedFiles = pNode;
    }
    __catch
    {
        __rethrow;
    }
}

static void handleUSR(Assembler* pThis)
{
    SizedString fullFilename = SizedString_InitFromString(TextFile_GetFilename(pThis->pMainFile));
    SizedString filename = removeDirectoryAndSuffixFromFullFilename(&fullFilename);

    __try
    {
        SizedString remainingArguments = pThis->parsedLine.operands;
        unsigned short side;
        unsigned short track;
        unsigned short offset;
        unsigned short length;
        
        validateOperandWasProvided(pThis);
        
        /* NOTE: &remaningArguments is an in/out parameter in the following calls to getNextCommaSeparatedArgument()
                 so that it points after the next comma in the argument list after parsing out the previous argument. */
        side = getNextCommaSeparatedArgument(pThis, &remainingArguments);
        track = getNextCommaSeparatedArgument(pThis, &remainingArguments);
        offset = getNextCommaSeparatedArgument(pThis, &remainingArguments);
        length = getNextCommaSeparatedArgument(pThis, &remainingArguments);
        if (SizedString_strlen(&remainingArguments) != 0)
            __throw(invalidArgumentCountException);

        BinaryBuffer_QueueRW18WriteToFile(pThis->pObjectBuffer, 
                                          pThis->pInitParams ? pThis->pInitParams->pOutputDirectory : NULL, 
                                          &filename,
                                          ".usr",
                                          side, track, offset);
    }
    __catch
    {
        if (getExceptionCode() == invalidArgumentCountException)
            LOG_ERROR(pThis, "'%.*s' doesn't contain the 4 arguments required for USR directive.",
                      pThis->parsedLine.operands.stringLength, pThis->parsedLine.operands.pString);
        else if (getExceptionCode() != missingOperandException)
            LOG_ERROR(pThis, "Failed to queue up USR save to '%.*s.usr'.", 
                      filename.stringLength, filename.pString);
        __nothrow;
    }
}

static unsigned short getNextCommaSeparatedArgument(Assembler* pThis, SizedString* pRemainingArguments)
{
    SizedString beforeComma;
    SizedString afterComma;
    Expression  expression;
    
    __try
    {
        SizedString_SplitString(pRemainingArguments, ',', &beforeComma, &afterComma);
        if (SizedString_strlen(&beforeComma) == 0)
            __throw(invalidArgumentCountException);
        expression = ExpressionEval(pThis, &beforeComma);
    }
    __catch
    {
        __rethrow;
    }
    
    *pRemainingArguments = afterComma;
    return expression.value;
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


static SizedString initGlobalLabelString(Assembler* pThis, SizedString* pLabelName);
__throws Symbol* Assembler_FindLabel(Assembler* pThis, SizedString* pLabelName)
{
    Symbol*     pSymbol = NULL;
    
    __try
    {
        SizedString globalLabel = initGlobalLabelString(pThis, pLabelName);
        SizedString localLabel = initLocalLabelString(pLabelName);

        validateLabelFormat(pThis, pLabelName);
        pSymbol = SymbolTable_Find(pThis->pSymbols, &globalLabel, &localLabel);
        if (!pSymbol)
            pSymbol = SymbolTable_Add(pThis->pSymbols, &globalLabel, &localLabel);
        if (!isSymbolAlreadyDefined(pSymbol, NULL))
            Symbol_LineReferenceAdd(pSymbol, pThis->pLineInfo);
    }
    __catch
    {
        __rethrow;
    }

    return pSymbol;
}

static SizedString initGlobalLabelString(Assembler* pThis, SizedString* pLabelName)
{
    if (!isGlobalLabelName(pLabelName))
        return pThis->globalLabel;

    return *pLabelName;
}
