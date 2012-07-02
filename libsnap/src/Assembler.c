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
//#include "SymbolTable.h"
#include "ParseLine.h"
#include "ListFile.h"
#include "LineBuffer.h"


#define NUMBER_OF_SYMBOL_TABLE_HASH_BUCKETS 511


struct Assembler
{
    TextFile*     pTextFile;
//    SymbolTable* pSymbols;
    ListFile*     pListFile;
    LineBuffer*   pLineText;
    LineInfo      lineInfo;
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
        // UNDONE: Take list file location.
        __throwing_func( pThis->pListFile = ListFile_Create(stdout) );
        __throwing_func( pThis->pLineText = LineBuffer_Create() );
//        pThis->pSymbols = SymbolTable_Create(NUMBER_OF_SYMBOL_TABLE_HASH_BUCKETS);
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
//    SymbolTable_Free(pThis->pSymbols);
    TextFile_Free(pThis->pTextFile);
    free(pThis);
}


static void parseSource(Assembler* pThis);
static void parseLine(Assembler* pThis, char* pLine);
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
    ParsedLine parsedLine;

    __try
    {
        __throwing_func( LineBuffer_Set(pThis->pLineText, pLine) );
        ParseLine(&parsedLine, pLine);
        listLine(pThis);
    }
    __catch
    {
        __rethrow;
    }
}

static void listLine(Assembler* pThis)
{
    pThis->lineInfo.pLineText = LineBuffer_Get(pThis->pLineText);
    pThis->lineInfo.lineNumber++;
    ListFile_OutputLine(pThis->pListFile, &pThis->lineInfo);
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
