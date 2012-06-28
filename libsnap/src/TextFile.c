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
#include "TextFile.h"
#include "TextFileTest.h"

struct TextFile
{
    char* pFileBuffer;
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


static long getTextLength(FILE* pFile);
static long adjustFileSizeToAllowForTrailingNull(long actualFileSize);
static char* allocateTextBuffer(long textLength);
static void readFileContentIntoTextBufferAndNullTerminate(char* pTextBuffer, long textBufferSize, FILE* pFile);
__throws TextFile* TextFile_CreateFromFile(const char* pFilename)
{
    FILE*     pFile = NULL;
    long      textLength = -1;
    char*     pTextBuffer = NULL;
    TextFile* pThis = NULL;
    
    pFile = fopen(pFilename, "r");
    if (!pFile)
        __throw_and_return(fileNotFoundException, NULL);

    __try
    {
        __throwing_func( textLength = getTextLength(pFile) );
        __throwing_func( pTextBuffer = allocateTextBuffer(textLength) );
        __throwing_func( readFileContentIntoTextBufferAndNullTerminate(pTextBuffer, textLength, pFile) );
        __throwing_func( pThis = TextFile_CreateFromString(pTextBuffer) );
        pThis->pFileBuffer = pTextBuffer;
    }
    __catch
    {
        free(pTextBuffer);
        fclose(pFile);
        __rethrow_and_return(NULL);
    }
    
    fclose(pFile);
    return pThis;
}

static long getTextLength(FILE* pFile)
{
    long fileSize;
    int  result;
    
    result = fseek(pFile, 0, SEEK_END);
    if (result != 0)
        __throw_and_return(fileException, -1);
        
    fileSize = ftell(pFile);
    if (fileSize == -1)
        __throw_and_return(fileException, -1);
        
    result = fseek(pFile, 0, SEEK_SET);
    if (result != 0)
        __throw_and_return(fileException, -1);
        
    return adjustFileSizeToAllowForTrailingNull(fileSize);
}

static long adjustFileSizeToAllowForTrailingNull(long actualFileSize)
{
    return actualFileSize + 1;
}

static char* allocateTextBuffer(long textLength)
{
    char* pTextBuffer;
    
    pTextBuffer = malloc(textLength);
    if (!pTextBuffer)
        __throw_and_return(outOfMemoryException, NULL);
        
    return pTextBuffer;
}

static void readFileContentIntoTextBufferAndNullTerminate(char* pTextBuffer, long textBufferSize, FILE* pFile)
{
    int fileContentSize = textBufferSize - 1;
    int result;
    
    result = fread(pTextBuffer, 1, fileContentSize, pFile);
    if (result != fileContentSize)
        __throw(fileException);
        
    pTextBuffer[textBufferSize - 1] = '\0';
}


void TextFile_Free(TextFile* pThis)
{
    if (!pThis)
        return;
    
    free(pThis->pFileBuffer);
    free(pThis);
}


static void nullTerminateLineAndAdvanceToNextLine(TextFile* pThis);
static char* findEndOfLine(TextFile* pThis);
static int isLineEndCharacter(char ch);
static char* findStartOfNextLine(char* pEndOfCurrentLine);
char* TextFile_GetNextLine(TextFile* pThis)
{
    char* pStartOfLine = pThis->pCurr;
    
    if (*pStartOfLine == '\0')
        return NULL;
    
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

