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
#include "LupSource.h"
#include "LupSourceTest.h"
#include "TextSourcePriv.h"
#include "util.h"

typedef struct LupSource
{
    TextSource     super;
    unsigned short loopIterations;
} LupSource;

static void freeObject(void *pvThis);
static SizedString getNextLine(void* pvThis);
static int isEndOfFile(void* pvThis);
static int isEndOfFile(void* pvThis);
static unsigned int getLineNumber(void* pvThis);
static const char* getFilename(void* pvThis);
static int shouldStartNextIteration(LupSource* pThis);

static TextSourceVTable g_vtable =
{
    freeObject,
    getNextLine,
    isEndOfFile,
    getLineNumber,
    getFilename
};


__throws TextSource* LupSource_Create(TextFile* pTextFile, unsigned short loopIterations)
{
    LupSource* pThis = NULL;
    
    __try
    {
        pThis = allocateAndZero(sizeof(*pThis));
        pThis->super.pVTable = &g_vtable;
        pThis->loopIterations = loopIterations;
        TextSource_SetTextFile((TextSource*)pThis, pTextFile);
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
    LupSource* pThis = (LupSource*)pvThis;
    free(pThis);
}

static SizedString getNextLine(void* pvThis)
{
    LupSource* pThis = (LupSource*)pvThis;
    if (shouldStartNextIteration(pThis))
    {
        TextFile_Reset(pThis->super.pTextFile);
        pThis->loopIterations--;
    }
    return TextFile_GetNextLine(pThis->super.pTextFile);
}

static int shouldStartNextIteration(LupSource* pThis)
{
    return TextFile_IsEndOfFile(pThis->super.pTextFile) && pThis->loopIterations > 1;
}

static int isEndOfFile(void* pvThis)
{
    LupSource* pThis = (LupSource*)pvThis;
    return TextFile_IsEndOfFile(pThis->super.pTextFile) && pThis->loopIterations == 1;
}

static unsigned int getLineNumber(void* pvThis)
{
    LupSource* pThis = (LupSource*)pvThis;
    return TextFile_GetLineNumber(pThis->super.pTextFile);
}

static const char* getFilename(void* pvThis)
{
    LupSource* pThis = (LupSource*)pvThis;
    return TextFile_GetFilename(pThis->super.pTextFile);
}
