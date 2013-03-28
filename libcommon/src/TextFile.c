/*  Copyright (C) 2013  Adam Green (https://github.com/adamgreen)

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
#include "util.h"

struct TextFile
{
    const TextFile* pBaseTextFile;
    char*           pFileBuffer;
    const char*     pText;
    const char*     pPrev;
    const char*     pCurr;
    const char*     pEnd;
    char*           pFilename;
    unsigned int    lineNumber;
    unsigned int    startLineNumber;
};


__throws static void initObject(TextFile* pThis, const char* pText);
__throws static char* allocateStringAndCopyMergedFilename(const SizedString* pDirectory, 
                                                          const SizedString* pFilename, 
                                                          const char*        pFilenameSuffix);
__throws TextFile* TextFile_CreateFromString(const char* pText)
{
    static const char        defaultFilename[] = "filename";
    static const SizedString filenameString = { defaultFilename, sizeof(defaultFilename) - 1 };
    TextFile*                pThis = NULL;
    
    __try
    {
        pThis = allocateAndZero(sizeof(*pThis));
        initObject(pThis, pText);
        pThis->pEnd = (char*)~0UL;
        pThis->pFilename = allocateStringAndCopyMergedFilename(NULL, &filenameString, NULL);
    }
    __catch
    {
        TextFile_Free(pThis);
        __rethrow;
    }
    
    return pThis;
}

__throws static void initObject(TextFile* pThis, const char* pText)
{
    pThis->pText = pText;
    pThis->pPrev = pText;
    pThis->pCurr = pText;
}

__throws static char* allocateStringAndCopyMergedFilename(const SizedString* pDirectory, 
                                                          const SizedString* pFilename, 
                                                          const char*        pFilenameSuffix)
{
    static const char pathSeparator = PATH_SEPARATOR;
    size_t filenameLength = SizedString_strlen(pFilename);
    size_t directoryLength = pDirectory ? SizedString_strlen(pDirectory) : 0;
    size_t roomForSlash = !pDirectory ? 0 : (pDirectory->pString[directoryLength-1] == PATH_SEPARATOR ? 0 : 1);
    size_t suffixLength = pFilenameSuffix ? strlen(pFilenameSuffix) : 0;
    size_t fullFilenameLength = directoryLength + roomForSlash + filenameLength + suffixLength + 1;
    char*  pFullFilename = malloc(fullFilenameLength);
    if (!pFullFilename)
        __throw(outOfMemoryException);

    if (pDirectory)
        memcpy(pFullFilename, pDirectory->pString, directoryLength);
    memcpy(pFullFilename + directoryLength, &pathSeparator, roomForSlash);
    memcpy(pFullFilename + directoryLength + roomForSlash, pFilename->pString, filenameLength);
    memcpy(pFullFilename + directoryLength + roomForSlash + filenameLength, pFilenameSuffix, suffixLength);
    pFullFilename[fullFilenameLength - 1] = '\0';
    
    return pFullFilename;
}


static FILE* openFile(const char* pFilename);
static long getTextLength(FILE* pFile);
static char* allocateTextBuffer(long textLength);
static void readFileContentIntoTextBuffer(char* pTextBuffer, long fileSize, FILE* pFile);
__throws TextFile* TextFile_CreateFromFile(const SizedString* pDirectory, 
                                           const SizedString* pFilename, 
                                           const char*        pFilenameSuffix)
{
    FILE*     pFile = NULL;
    long      textLength = -1;
    TextFile* pThis = NULL;
    
    __try
    {
        pThis = allocateAndZero(sizeof(*pThis));
        pThis->pFilename = allocateStringAndCopyMergedFilename(pDirectory, pFilename, pFilenameSuffix);
        pFile = openFile(pThis->pFilename);
        textLength = getTextLength(pFile);
        pThis->pFileBuffer = allocateTextBuffer(textLength);
        pThis->pEnd = pThis->pFileBuffer + textLength;
        readFileContentIntoTextBuffer(pThis->pFileBuffer, textLength, pFile);
        initObject(pThis, pThis->pFileBuffer);
    }
    __catch
    {
        TextFile_Free(pThis);
        if (pFile)
            fclose(pFile);
        __rethrow;
    }
    
    fclose(pFile);
    return pThis;
}

static FILE* openFile(const char* pFilename)
{
    FILE* pFile = NULL;
    
    pFile = fopen(pFilename, "rb");
    if (!pFile)
        __throw(fileNotFoundException);
    return pFile;
}

static long getTextLength(FILE* pFile)
{
    long fileSize;
    int  result;
    
    result = fseek(pFile, 0, SEEK_END);
    if (result != 0)
        __throw(fileException);
        
    fileSize = ftell(pFile);
    if (fileSize == -1)
        __throw(fileException);
        
    result = fseek(pFile, 0, SEEK_SET);
    if (result != 0)
        __throw(fileException);
        
    return fileSize;
}

static char* allocateTextBuffer(long textLength)
{
    char* pTextBuffer;
    
    pTextBuffer = malloc(textLength);
    if (!pTextBuffer)
        __throw(outOfMemoryException);
        
    return pTextBuffer;
}

static void readFileContentIntoTextBuffer(char* pTextBuffer, long fileSize, FILE* pFile)
{
    int result;
    
    result = fread(pTextBuffer, 1, fileSize, pFile);
    if (result != fileSize)
        __throw(fileException);
}


__throws TextFile* TextFile_CreateFromTextFile(const TextFile* pTextFile)
{
    TextFile* pThis = allocateAndZero(sizeof(*pThis));
    *pThis = *pTextFile;
    pThis->pText = pTextFile->pCurr;
    pThis->startLineNumber = pTextFile->lineNumber;
    pThis->pBaseTextFile = pTextFile;
    
    return pThis;
}


static int isDerivedTextFile(TextFile* pThis);
void TextFile_Free(TextFile* pThis)
{
    if (!pThis)
        return;
    
    if (!isDerivedTextFile(pThis))
    {
        free(pThis->pFileBuffer);
        free(pThis->pFilename);
    }
    free(pThis);
}

static int isDerivedTextFile(TextFile* pThis)
{
    return pThis->pBaseTextFile != NULL;
}


void TextFile_Reset(TextFile* pThis)
{
    initObject(pThis, pThis->pText);
    pThis->lineNumber = pThis->startLineNumber;
}


void TextFile_SetEndOfFile(TextFile* pThis)
{
    pThis->pEnd = pThis->pPrev;    
}


void TextFile_AdvanceTo(TextFile* pThis, const TextFile* pAdvanceToMatch)
{
    if (pAdvanceToMatch->pBaseTextFile != pThis)
        __throw(invalidArgumentException);
    
    pThis->pCurr = pAdvanceToMatch->pPrev;
    pThis->pPrev = pAdvanceToMatch->pPrev;
    pThis->lineNumber = pAdvanceToMatch->lineNumber ? pAdvanceToMatch->lineNumber - 1 : 0;
}


static int isEndOfFile(TextFile* pThis);
static const char* findEndOfLine(TextFile* pThis);
static int isLineEndCharacter(char ch);
static void advanceToNextLine(TextFile* pThis);
SizedString TextFile_GetNextLine(TextFile* pThis)
{
    const char* pStartOfLine = pThis->pCurr;
    const char* pEndOfLine;
    
    if (isEndOfFile(pThis))
        return SizedString_InitFromString(NULL);
    
    pThis->pPrev = pThis->pCurr;
    pEndOfLine = findEndOfLine(pThis);
    advanceToNextLine(pThis);
    pThis->lineNumber++;
    
    return SizedString_Init(pStartOfLine, pEndOfLine - pStartOfLine);
}

static int isEndOfFile(TextFile* pThis)
{
    return *pThis->pCurr == '\0' || pThis->pCurr >= pThis->pEnd;
}

static const char* findEndOfLine(TextFile* pThis)
{
    while (!isEndOfFile(pThis) && !isLineEndCharacter(*pThis->pCurr))
        pThis->pCurr++;
    
    /* Get here if *pCurr is '\0' or '\r' or '\n' */
    return pThis->pCurr;
}

static int isLineEndCharacter(char ch)
{
    return ch == '\r' || ch == '\n';
}

static void advanceToNextLine(TextFile* pThis)
{
    char  prev = *pThis->pCurr;
    char  curr;
    
    if (isEndOfFile(pThis))
        return;
    
    curr = pThis->pCurr[1];
    
    if ((prev == '\r' && curr == '\n') ||
        (prev == '\n' && curr == '\r'))
    {
        pThis->pCurr += 2;
        return;
    }
    
    pThis->pCurr += 1;
}


int TextFile_IsEndOfFile(TextFile* pThis)
{
    return isEndOfFile(pThis);
}


unsigned int TextFile_GetLineNumber(TextFile* pThis)
{
    return pThis->lineNumber;
}


const char* TextFile_GetFilename(TextFile* pThis)
{
    return pThis->pFilename;
}
