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
#include <stdio.h>
#include "Assembler.h"
#include "AssemblerTest.h"
#include "TextFile.h"
#include "SymbolTable.h"
#include "ParseLine.h"
#include "ListFile.h"
#include "LineBuffer.h"
#include "util.h"


#define NUMBER_OF_SYMBOL_TABLE_HASH_BUCKETS 511


struct Assembler
{
    const char*    pSourceFilename;
    TextFile*      pTextFile;
    SymbolTable* pSymbols;
    ListFile*      pListFile;
    LineBuffer*    pLineText;
    ParsedLine     parsedLine;
    LineInfo       lineInfo;
    unsigned int   errorCount;
    unsigned short programCounter;
};


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
        pThis->pSymbols = SymbolTable_Create(NUMBER_OF_SYMBOL_TABLE_HASH_BUCKETS);
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


void Assembler_Free(Assembler* pThis)
{
    if (!pThis)
        return;
    
    LineBuffer_Free(pThis->pLineText);
    ListFile_Free(pThis->pListFile);
    SymbolTable_Free(pThis->pSymbols);
    TextFile_Free(pThis->pTextFile);
    free(pThis);
}


static void parseSource(Assembler* pThis);
static void parseLine(Assembler* pThis, char* pLine);
static void prepareLineInfoForThisLine(Assembler* pThis);
static void firstPassAssembleLine(Assembler* pThis);
static void handleEQU(Assembler* pThis);
static unsigned short evaluateExpression(Assembler* pThis);
static int isHexValue(const char* pValue);
static void attemptToAddSymbol(Assembler* pThis);
static void ignoreOperator(Assembler* pThis);
static void handleInvalidOperator(Assembler* pThis);
static void handleHEX(Assembler* pThis);
static unsigned char getNextHexByte(const char* pStart, const char** ppNext);
static unsigned char hexCharToNibble(char value);
static void logHexParseError(Assembler* pThis);
static void handleORG(Assembler* pThis);
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
        prepareLineInfoForThisLine(pThis);
        ParseLine(&pThis->parsedLine, pLine);
        firstPassAssembleLine(pThis);
        listLine(pThis);
    }
    __catch
    {
        __rethrow;
    }
}

static void prepareLineInfoForThisLine(Assembler* pThis)
{
    unsigned int lineNumber = pThis->lineInfo.lineNumber;
    
    memset(&pThis->lineInfo, 0, sizeof(pThis->lineInfo));
    pThis->lineInfo.lineNumber = lineNumber + 1;
    pThis->lineInfo.pLineText = LineBuffer_Get(pThis->pLineText);
    pThis->lineInfo.address = pThis->programCounter;
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
        {"ORG", handleORG}
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
        __throwing_func( pThis->lineInfo.symbolValue = evaluateExpression(pThis) );
        __throwing_func( attemptToAddSymbol(pThis) );
    }
    __catch
    {
        __nothrow;
    }

    pThis->lineInfo.validSymbol = 1;
    
}

#define LOG_ERROR(FORMAT, ...) fprintf(stderr, \
                                       "%s:%d: error: " FORMAT LINE_ENDING, \
                                       pThis->pSourceFilename, \
                                       pThis->lineInfo.lineNumber, \
                                       __VA_ARGS__), pThis->errorCount++
                                       
// UNDONE: Refactor.
static unsigned short evaluateExpression(Assembler* pThis)
{
    Symbol* pSymbol = NULL;
    
    if (isHexValue(pThis->parsedLine.pOperands))
    {
        return strtoul(&pThis->parsedLine.pOperands[1], NULL, 16);
    }
    else if (NULL != (pSymbol = SymbolTable_Find(pThis->pSymbols, pThis->parsedLine.pOperands)))
    {
        return pSymbol->value;
    }
    else
    {
        LOG_ERROR("Unexpected prefix in '%s' expression.", pThis->parsedLine.pOperands);
        __throw_and_return(invalidArgumentException, 0);
    }
}

static int isHexValue(const char* pValue)
{
    return *pValue == '$';
}

static void attemptToAddSymbol(Assembler* pThis)
{
    Symbol* pSymbol = NULL;
    
    if (SymbolTable_Find(pThis->pSymbols, pThis->parsedLine.pLabel))
    {
        LOG_ERROR("'%s' symbol has already been defined.", pThis->parsedLine.pLabel);
        __throw(invalidArgumentException);
    }
    
    __try
    {
        pSymbol = SymbolTable_Add(pThis->pSymbols, pThis->parsedLine.pLabel);
    }
    __catch
    {
        LOG_ERROR("Failed to allocate space for '%s' symbol.", pThis->parsedLine.pLabel);
        __rethrow;
    }
    
    pSymbol->value = pThis->lineInfo.symbolValue;
}

static void ignoreOperator(Assembler* pThis)
{
}

static void handleInvalidOperator(Assembler* pThis)
{
    LOG_ERROR("'%s' is not a recongized directive, mnemonic, or macro.", pThis->parsedLine.pOperator);
}

static void handleHEX(Assembler* pThis)
{
    const char* pCurr = pThis->parsedLine.pOperands;
    size_t      i = 0;

    while (*pCurr && i < sizeof(pThis->lineInfo.machineCode))
    {
        __try
        {
            unsigned int byte;
            const char*  pNext;

            __throwing_func( byte = getNextHexByte(pCurr, &pNext) );
            pThis->lineInfo.machineCode[i++] = byte;
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
        LOG_ERROR("'%s' contains more than 32 values.", pThis->parsedLine.pOperands);
        return;
    }
    pThis->lineInfo.machineCodeSize = i;
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
        LOG_ERROR("'%s' doesn't contain an even number of hex digits.", pThis->parsedLine.pOperands);
    else if (getExceptionCode() == invalidHexDigitException)
        LOG_ERROR("'%s' contains an invalid hex digit.", pThis->parsedLine.pOperands);
}

static void handleORG(Assembler* pThis)
{
    pThis->programCounter = evaluateExpression(pThis);
}

static void listLine(Assembler* pThis)
{
    ListFile_OutputLine(pThis->pListFile, &pThis->lineInfo);
}


unsigned int Assembler_GetErrorCount(Assembler* pThis)
{
    return pThis->errorCount;
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
