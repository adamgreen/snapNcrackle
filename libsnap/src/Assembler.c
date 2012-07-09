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
#include "AssemblerPriv.h"
#include "ExpressionEval.h"


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


static void parseSource(Assembler* pThis);
static void parseLine(Assembler* pThis, char* pLine);
static void prepareLineInfoForThisLine(Assembler* pThis, char* pLine);
static void firstPassAssembleLine(Assembler* pThis);
static void handleEQU(Assembler* pThis);
static void validateEQULabelFormat(Assembler* pThis);
static void validateLabelFormat(Assembler* pThis);
static Symbol* attemptToAddSymbol(Assembler* pThis, const char* pSymbolName);
static void ignoreOperator(Assembler* pThis);
static void handleInvalidOperator(Assembler* pThis);
static void handleHEX(Assembler* pThis);
static unsigned char getNextHexByte(const char* pStart, const char** ppNext);
static unsigned char hexCharToNibble(char value);
static void logHexParseError(Assembler* pThis);
static void handleORG(Assembler* pThis);
static int isTypeAbsolute(Expression* pExpression);
static void handleLDA(Assembler* pThis);
static void logInvalidAddressingMode(Assembler* pThis);
static void emitImmediateInstruction(Assembler* pThis, unsigned char opCode, Expression* pExpression);
static void handleSTA(Assembler* pThis);
static void emitAbsoluteInstruction(Assembler* pThis, unsigned char opCode, Expression* pExpression);
static void emitZeroPageAbsoluteInstruction(Assembler* pThis, unsigned char opCode, Expression* pExpression);
static void handleJSR(Assembler* pThis);
static void handleLDX(Assembler* pThis);
static void handleTXA(Assembler* pThis);
static void emitImpliedInstruction(Assembler* pThis, unsigned char opCode);
static void handleLSR(Assembler* pThis);
static void handleORA(Assembler* pThis);
static void rememberLabel(Assembler* pThis);
static int isLabelToRemember(Assembler* pThis);
static int doesLineContainALabel(Assembler* pThis);
static int wasEQUDirective(Assembler* pThis);
static const char* expandedLabelName(Assembler* pThis, const char* pLabelName, size_t labelLength);
static int isGlobalLabelName(const char* pLabelName);
static int isLocalLabelName(const char* pLabelName);
static void expandLocalLabelToGloballyUniqueName(Assembler* pThis, const char* pLocalLabelName, size_t length);
static int seenGlobalLabel(Assembler* pThis);
static void rememberGlobalLabel(Assembler* pThis);
static void listLine(Assembler* pThis);
void Assembler_Run(Assembler* pThis)
{
    parseSource(pThis);
    return;
}

static void parseSource(Assembler* pThis)
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
        listLine(pThis);
        pThis->programCounter += pThis->pLineInfo->machineCodeSize;
    }
    __catch
    {
        __rethrow;
    }
}

static void prepareLineInfoForThisLine(Assembler* pThis, char* pLine)
{
    // UNDONE: Test for failed allocation.
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

static void firstPassAssembleLine(Assembler* pThis)
{
    size_t i;
    struct
    {
        const char* pOperator;
        void (*handler)(Assembler* pThis);
    } operatorHandlers[] =
    {
        {"=", handleEQU},
        {"EQU", handleEQU},
        {"LST", ignoreOperator},
        {"HEX", handleHEX},
        {"ORG", handleORG},
        {"LDA", handleLDA},
        {"STA", handleSTA},
        {"JSR", handleJSR},
        {"LDX", handleLDX},
        {"TXA", handleTXA},
        {"LSR", handleLSR},
        {"ORA", handleORA}
    };
    
    
    if (pThis->parsedLine.pOperator == NULL)
        return;
    
    for (i = 0 ; i < ARRAYSIZE(operatorHandlers) ; i++)
    {
        if (0 == strcasecmp(pThis->parsedLine.pOperator, operatorHandlers[i].pOperator))
        {
            operatorHandlers[i].handler(pThis);
            return;
        }
    }
    
    handleInvalidOperator(pThis);
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
        __throwing_func( pSymbol = attemptToAddSymbol(pThis, pThis->parsedLine.pLabel) );
        pSymbol->expression = expression;
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

static Symbol* attemptToAddSymbol(Assembler* pThis, const char* pSymbolName)
{
    Symbol* pSymbol = NULL;
    if (SymbolTable_Find(pThis->pSymbols, pSymbolName))
    {
        LOG_ERROR(pThis, "'%s' symbol has already been defined.", pSymbolName);
        __throw_and_return(invalidArgumentException, NULL);
    }
    
    __try
    {
        __throwing_func( pSymbol = SymbolTable_Add(pThis->pSymbols, pSymbolName) );
    }
    __catch
    {
        LOG_ERROR(pThis, "Failed to allocate space for '%s' symbol.", pSymbolName);
        __rethrow_and_return(NULL);
    }
    
    return pSymbol;
}

static void ignoreOperator(Assembler* pThis)
{
}

static void handleInvalidOperator(Assembler* pThis)
{
    LOG_ERROR(pThis, "'%s' is not a recongized directive, mnemonic, or macro.", pThis->parsedLine.pOperator);
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
    return pExpression->type == TYPE_ZEROPAGE_ABSOLUTE ||
           pExpression->type == TYPE_ABSOLUTE;
}

static void handleLDA(Assembler* pThis)
{
    Expression expression;
    
    __try
        expression = ExpressionEval(pThis, pThis->parsedLine.pOperands);
    __catch
        __nothrow;
    
    if (expression.type != TYPE_IMMEDIATE)
    {
        logInvalidAddressingMode(pThis);
        return;
    }
    
    emitImmediateInstruction(pThis, 0xa9, &expression);
}

static void logInvalidAddressingMode(Assembler* pThis)
{
    LOG_ERROR(pThis, "'%s' specifies invalid addressing mode for this instruction.", pThis->parsedLine.pOperands);
}

static void emitImmediateInstruction(Assembler* pThis, unsigned char opCode, Expression* pExpression)
{
    pThis->pLineInfo->machineCode[0] = opCode;
    pThis->pLineInfo->machineCode[1] = LO_BYTE(pExpression->value);
    pThis->pLineInfo->machineCodeSize = 2;
}

static void handleSTA(Assembler* pThis)
{
    Expression expression;
    
    __try
        expression = ExpressionEval(pThis, pThis->parsedLine.pOperands);
    __catch
        __nothrow;
    
    switch (expression.type)
    {
    case TYPE_ZEROPAGE_ABSOLUTE:
        emitZeroPageAbsoluteInstruction(pThis, 0x85, &expression);
        return;
    case TYPE_ABSOLUTE:
        emitAbsoluteInstruction(pThis, 0x8d, &expression);
        return;
    default:
        logInvalidAddressingMode(pThis);
        return;
    }
}

static void emitAbsoluteInstruction(Assembler* pThis, unsigned char opCode, Expression* pExpression)
{
    pThis->pLineInfo->machineCode[0] = opCode;
    pThis->pLineInfo->machineCode[1] = LO_BYTE(pExpression->value);
    pThis->pLineInfo->machineCode[2] = HI_BYTE(pExpression->value);
    pThis->pLineInfo->machineCodeSize = 3;
}

static void emitZeroPageAbsoluteInstruction(Assembler* pThis, unsigned char opCode, Expression* pExpression)
{
    pThis->pLineInfo->machineCode[0] = opCode;
    pThis->pLineInfo->machineCode[1] = LO_BYTE(pExpression->value);
    pThis->pLineInfo->machineCodeSize = 2;
}

static void handleJSR(Assembler* pThis)
{
    Expression expression;
    
    __try
        expression = ExpressionEval(pThis, pThis->parsedLine.pOperands);
    __catch
        __nothrow;
    
    switch (expression.type)
    {
    case TYPE_ZEROPAGE_ABSOLUTE:
    case TYPE_ABSOLUTE:
        emitAbsoluteInstruction(pThis, 0x20, &expression);
        return;
    default:
        logInvalidAddressingMode(pThis);
        return;
    }
}

static void handleLDX(Assembler* pThis)
{
    Expression expression;
    
    __try
        expression = ExpressionEval(pThis, pThis->parsedLine.pOperands);
    __catch
        __nothrow;
    
    switch (expression.type)
    {
    case TYPE_ZEROPAGE_ABSOLUTE:
        emitZeroPageAbsoluteInstruction(pThis, 0xa6, &expression);
        return;
    case TYPE_ABSOLUTE:
        emitAbsoluteInstruction(pThis, 0xae, &expression);
        return;
    default:
        logInvalidAddressingMode(pThis);
        return;
    }
}

static void handleTXA(Assembler* pThis)
{
    emitImpliedInstruction(pThis, 0x8a);
}

static void emitImpliedInstruction(Assembler* pThis, unsigned char opCode)
{
    pThis->pLineInfo->machineCode[0] = opCode;
    pThis->pLineInfo->machineCodeSize = 1;
}

static void handleLSR(Assembler* pThis)
{
    if (pThis->parsedLine.pOperands)
    {
        logInvalidAddressingMode(pThis);
        return;
    }
    emitImpliedInstruction(pThis, 0x4a);
}

static void handleORA(Assembler* pThis)
{
    Expression expression;
    
    __try
        expression = ExpressionEval(pThis, pThis->parsedLine.pOperands);
    __catch
        __nothrow;
    
    if (expression.type != TYPE_IMMEDIATE)
    {
        logInvalidAddressingMode(pThis);
        return;
    }
    
    emitImmediateInstruction(pThis, 0x09, &expression);
}

static void rememberLabel(Assembler* pThis)
{
    if (!isLabelToRemember(pThis))
        return;

    __try
    {
        const char* pExpandedLabelName = NULL;
        Symbol*     pSymbol = NULL;
    
        __throwing_func( validateLabelFormat(pThis) );
        __throwing_func( pExpandedLabelName = expandedLabelName(pThis, pThis->parsedLine.pLabel, strlen(pThis->parsedLine.pLabel)) );
        __throwing_func( pSymbol = attemptToAddSymbol(pThis, pExpandedLabelName) );
        __throwing_func( rememberGlobalLabel(pThis) );
        pSymbol->expression = ExpressionEval_CreateAbsoluteExpression(pThis->programCounter);
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
    if (isGlobalLabelName(pLabelName))
        return pLabelName;
    
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

static void expandLocalLabelToGloballyUniqueName(Assembler* pThis, const char* pLocalLabelName, size_t length)
{
    if (!seenGlobalLabel(pThis))
    {
        LOG_ERROR(pThis, "'%.*s' local label isn't allowed before first global label.", length, pLocalLabelName);
        __throw(invalidArgumentException);
    }
    if (length > pThis->maxLocalLabelSize)
    {
        LOG_ERROR(pThis, "'%.*s' label is too long.", length, pLocalLabelName);
        __throw(bufferOverrunException);
    }
    
    memcpy(pThis->pLocalLabelStart, pLocalLabelName, length);
    pThis->pLocalLabelStart[length] = '\0';
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

    if (length >= sizeof(pThis->labelBuffer))
    {
        LOG_ERROR(pThis, "'%s' label is too long.", pThis->parsedLine.pLabel);
        __throw(bufferOverrunException);
    }
    memcpy(pThis->labelBuffer, pThis->parsedLine.pLabel, length);
    pThis->pLocalLabelStart = &pThis->labelBuffer[length];
    pThis->maxLocalLabelSize = sizeof(pThis->labelBuffer) - length - 1;
}

static void listLine(Assembler* pThis)
{
    ListFile_OutputLine(pThis->pListFile, pThis->pLineInfo);
}


unsigned int Assembler_GetErrorCount(Assembler* pThis)
{
    return pThis->errorCount;
}


__throws Symbol* Assembler_FindLabel(Assembler* pThis, const char* pLabelName, size_t labelLength)
{
    const char* pExpandedlName;
    
    __try
        __throwing_func( pExpandedlName = expandedLabelName(pThis, pLabelName, labelLength) );
    __catch
        __rethrow_and_return(NULL);

    return SymbolTable_Find(pThis->pSymbols, pExpandedlName);
}







#ifdef UNDONE
static void rememberNewOperators(Assembler* pThis, const char* pOperator)
{
    Symbol* pSymbol;
    
    if (!pOperator)
        return;
        
    pSymbol = SymbolTable_Find(pThis->pSymbols, pOperator);
    if (pSymbol)
        return;
        
    __try
        SymbolTable_Add(pThis->pSymbols, pOperator, NULL);
    __catch
        __rethrow;
}

static void listOperators(Assembler* pThis)
{
    Symbol* pSymbol;
    
    printf("Operators encountered in this assembly language file.\r\n");
    SymbolTable_EnumStart(pThis->pSymbols);
    while (NULL != (pSymbol = SymbolTable_EnumNext(pThis->pSymbols)))
    {
        printf("%s\r\n", pSymbol->pKey);
    }
}


__throws Assembler* Assembler_CreateFromCommandLine(CommandLine* pCommandLine)
{
    Assembler* pThis = NULL;
    
    __try
    {
        __throwing_func( pThis = allocateObject() );
        __throwing_func( commonObjectInit(pThis) );
        pThis->pCommandLine = pCommandLine;
    }
    __catch
    {
        Assembler_Free(pThis);
        __rethrow_and_return(NULL);
    }

    return pThis;
}


typedef struct ListNode
{
    TextFile*        pFileToFree;
    struct ListNode* pNext;
} ListNode;

void Assembler_RunMultiple(Assembler* pThis)
{
    ListNode* pFilesToFree = NULL;
    int       i;
    
    for (i = 0 ; i < pThis->pCommandLine->sourceFileCount ; i++)
    {
        ListNode* pNode;
        
        __try
        {
            pThis->pTextFile = TextFile_CreateFromFile(pThis->pCommandLine->pSourceFiles[i]);
        }
        __catch
        {
            printf("Failed to open %s\n", pThis->pCommandLine->pSourceFiles[i]);
            __rethrow;
        }
    
        parseSource(pThis);
        
        /* Using this hack to keep the symbols around across files. */
        pNode = malloc(sizeof(*pNode));
        pNode->pFileToFree = pThis->pTextFile;
        pNode->pNext = pFilesToFree;
        pFilesToFree = pNode;
        pThis->pTextFile = NULL;
    }
    
    listOperators(pThis);
    
    /* Free up all of the files now that we are done with their symbols. */
    while (pFilesToFree)
    {
        ListNode* pNext = pFilesToFree->pNext;
        TextFile_Free(pFilesToFree->pFileToFree);
        free(pFilesToFree);
        pFilesToFree = pNext;
    }
    
    return;
}

#endif /* UNDONE */
