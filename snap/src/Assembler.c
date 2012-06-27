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

struct Assembler
{
    TextFile*    pTextFile;
    SymbolTable* pSymbols;
};


static void initObjectFromString(Assembler* pThis, char* pText);
__throws Assembler* Assembler_CreateFromString(char* pText)
{
    Assembler* pThis = malloc(sizeof(*pThis));
    if (!pThis)
        __throw_and_return(outOfMemoryException, NULL);
    
    __try
        initObjectFromString(pThis, pText);
    __catch
        __rethrow_and_return(NULL);

    return pThis;
}

static void initObjectFromString(Assembler* pThis, char* pText)
{
    memset(pThis, 0, sizeof(*pThis));
    __try
    {
        __throwing_func( pThis->pTextFile = TextFile_CreateFromString(pText) );
        __throwing_func( pThis->pSymbols = SymbolTable_Create(511) );
    }
    __catch
    {
        Assembler_Free(pThis);
        __rethrow;
    }
}

void Assembler_Free(Assembler* pThis)
{
    if (!pThis)
        return;
    
    SymbolTable_Free(pThis->pSymbols);
    TextFile_Free(pThis->pTextFile);
    free(pThis);
}


static void parseSource(Assembler* pThis);
static void parseLine(Assembler* pThis, char* pLine);
static void rememberNewOperators(Assembler* pThis, const char* pOperator);
void Assembler_Run(Assembler* pThis)
{
    printf("Operators encountered in this assembly language file.\r\n");
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

    ParseLine(&parsedLine, pLine);
    rememberNewOperators(pThis, parsedLine.pOperator);
}

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
    printf("%s\r\n", pOperator);
}
