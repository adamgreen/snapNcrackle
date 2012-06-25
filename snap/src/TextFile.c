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
#include "TextFile.h"
#include "TextFileTest.h"

struct TextFile
{
    char* pText;
    char* pCurr;
};


__throws static TextFile* allocateObject(void);
__throws static void initObject(TextFile* pThis, char* pText);
__throws TextFile* TextFile_CreateFromString(char* pText)
{
    TextFile* pThis = NULL;
    
    __try
        pThis = allocateObject();
    __catch
        __rethrow_and_return(NULL);

    initObject(pThis, pText);
    
    return pThis;
}

__throws static TextFile* allocateObject(void)
{
    TextFile* pThis = NULL;
    
    pThis = malloc(sizeof(*pThis));
    if (!pThis)
        __throw_and_return(outOfMemoryException, NULL);
    memset(pThis, 0, sizeof(*pThis));
    
    return pThis;
}

__throws static void initObject(TextFile* pThis, char* pText)
{
    pThis->pText = pText;
    pThis->pCurr = pText;
}


void TextFile_Free(TextFile* pThis)
{
    if (!pThis)
        return;
    
    free(pThis);
}


static void nullTerminateLineAndAdvanceToNextLine(TextFile* pThis);
static char* findEndOfLine(TextFile* pThis);
static int isLineEndCharacter(char ch);
static char* findStartOfNextLine(char* pEndOfCurrentLine);
__throws char* TextFile_GetNextLine(TextFile* pThis)
{
    char* pStartOfLine = pThis->pCurr;
    
    if (*pStartOfLine == '\0')
        __throw_and_return(endOfFileException, NULL);
    
    nullTerminateLineAndAdvanceToNextLine(pThis);
    
    return pStartOfLine;
}

static void nullTerminateLineAndAdvanceToNextLine(TextFile* pThis)
{
    char* pLineEnd = findEndOfLine(pThis);
    char* pNextLineStart = findStartOfNextLine(pLineEnd);
    
    *pLineEnd = '\0';
    pThis->pCurr = pNextLineStart;
}

static char* findEndOfLine(TextFile* pThis)
{
    char* pCurr = pThis->pCurr;
    
    while (*pCurr && !isLineEndCharacter(*pCurr))
        pCurr++;
    
    /* Get here if *pCurr is '\0' or '\r' or '\n' */
    return pCurr;
}

static int isLineEndCharacter(char ch)
{
    return ch == '\r' || ch == '\n';
}

static char* findStartOfNextLine(char* pEndOfCurrentLine)
{
    char  prev = *pEndOfCurrentLine;
    char  curr;
    
    if (prev == '\0')
        return pEndOfCurrentLine;
    
    curr = pEndOfCurrentLine[1];
    
    if ((prev == '\r' && curr == '\n') ||
        (prev == '\n' && curr == '\r'))
    {
        return pEndOfCurrentLine + 2;
    }
    
    return pEndOfCurrentLine + 1;
}

