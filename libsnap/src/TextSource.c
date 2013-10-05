/*  Copyright (C) 2013  Adam Green (https://github.com/adamgreen)
    Copyright (C) 2013  Tee-Kiah Chia

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
#include "TextSource.h"
#include "TextSourceTest.h"
#include "TextSourcePriv.h"

static TextSource* g_pFreeList;

int TextSource_IsEndOfFile(TextSource* pThis)
{
    return pThis->pVTable->isEndOfFile(pThis);
}


SizedString  TextSource_GetNextLine(TextSource* pThis)
{
    return pThis->pVTable->getNextLine(pThis);
}


unsigned int TextSource_GetLineNumber(TextSource* pThis)
{
    return pThis->pVTable->getLineNumber(pThis);
}


const char*  TextSource_GetFilename(TextSource* pThis)
{
    return pThis->pVTable->getFilename(pThis);
}

const char* TextSource_GetFirstFilename(TextSource* pThis)
{
    TextSource* pThat = pThis;
    while (pThat->stackDepth != 1)
        pThat = pThat->pStackPrev;
    return TextSource_GetFilename(pThat);
}

TextFile* TextSource_GetTextFile(TextSource* pThis)
{
    return pThis->pTextFile;
}

void TextSource_FreeAll(void)
{
    TextSource* pCurr = g_pFreeList;
    while(pCurr)
    {
        TextSource* pNext = pCurr->pFreeNext;
        TextFile_Free(pCurr->pTextFile);
        pCurr->pVTable->freeObject(pCurr);
        pCurr = pNext;
    }
    g_pFreeList = NULL;
}


void TextSource_AddToFreeList(TextSource* pObjectToAdd)
{
    pObjectToAdd->pFreeNext = g_pFreeList;
    g_pFreeList = pObjectToAdd;
}


void TextSource_StackPush(TextSource** ppTopOfStack, TextSource* pToPush)
{
    unsigned int prevStackDepth = *ppTopOfStack ? (*ppTopOfStack)->stackDepth : 0;
    pToPush->pStackPrev = *ppTopOfStack;
    pToPush->stackDepth = prevStackDepth + 1;
    *ppTopOfStack = pToPush;
}

void TextSource_StackPop(TextSource** ppTopOfStack)
{
    TextSource* pTopOfStack = *ppTopOfStack;
    if (!pTopOfStack)
        return;
    *ppTopOfStack = pTopOfStack->pStackPrev;
}

unsigned int TextSource_StackDepth(TextSource* pTopOfStack)
{
    if (!pTopOfStack)
        return 0;
    return pTopOfStack->stackDepth;
}

void TextSource_SetTextFile(TextSource* pTextSource, TextFile* pTextFile)
{
    pTextSource->pTextFile = pTextFile;
}
