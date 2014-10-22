/*  Copyright (C) 2014  Tee-Kiah Chia

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
#include "MacroExpansionSource.h"
#include "MallocFailureInject.h"
#include "TextSourcePriv.h"
#include "util.h"
#include <stdio.h>

typedef struct MacroExpansionSource
{
    TextSource     super;
    const char*    fileName;
    SizedString*   macroExpansionLines;
    unsigned int   startingSourceLine;
    unsigned short currentLineOffset;
    unsigned short numberOfLines;
} MacroExpansionSource;

static void freeObject(void *pvThis);
static SizedString getNextLine(void* pvThis);
static int isEndOfFile(void* pvThis);
static unsigned int getLineNumber(void* pvThis);
static const char* getFilename(void* pvThis);

static TextSourceVTable g_vtable =
{
    freeObject,
    getNextLine,
    isEndOfFile,
    getLineNumber,
    getFilename
};


__throws TextSource* MacroExpansionSource_Create(TextFile* pTextFile,
  unsigned int startingSourceLine, SizedString* macroExpansionLines,
  unsigned short numberOfLines)
{
    MacroExpansionSource* pThis = NULL;
    
    __try
    {
        pThis = allocateAndZero(sizeof(*pThis));
        pThis->super.pVTable = &g_vtable;
        pThis->fileName = TextFile_GetFilename(pTextFile);
        pThis->macroExpansionLines =
          allocateAndZero(numberOfLines * sizeof(SizedString));
        memcpy(pThis->macroExpansionLines, macroExpansionLines,
          numberOfLines * sizeof(SizedString));
        pThis->startingSourceLine = startingSourceLine;
        pThis->currentLineOffset = 0;
        pThis->numberOfLines = numberOfLines;
        TextSource_AddToFreeList((TextSource*)pThis);
    }
    __catch
    {
        freeObject(pThis);
        __rethrow;
    }
    
    return (TextSource*)pThis;
}


static void freeObject(void *pvThis)
{
    if (!pvThis)
        return;
    MacroExpansionSource* pThis = (MacroExpansionSource*)pvThis;
    free(pThis);
}

static SizedString getNextLine(void* pvThis)
{
    MacroExpansionSource* pThis = (MacroExpansionSource*)pvThis;
    if (isEndOfFile(pvThis))
        return SizedString_InitFromString("");
    return pThis->macroExpansionLines[pThis->currentLineOffset++];
}

static int isEndOfFile(void* pvThis)
{
    MacroExpansionSource* pThis = (MacroExpansionSource*)pvThis;
    return pThis->currentLineOffset >= pThis->numberOfLines;
}

static unsigned int getLineNumber(void* pvThis)
{
    MacroExpansionSource* pThis = (MacroExpansionSource*)pvThis;
    return pThis->startingSourceLine + pThis->currentLineOffset;
}

static const char* getFilename(void* pvThis)
{
    MacroExpansionSource* pThis = (MacroExpansionSource*)pvThis;
    return pThis->fileName;
}
