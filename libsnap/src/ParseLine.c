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
#include <ctype.h>
#include "ParseLine.h"


static int isLineEmptyOrFullLineComment(const SizedString* pLine, const char* pCurr);
static int isLineEmpty(const SizedString* pLine, const char* pCurr);
static int isFullLineComment(const SizedString* pLine, const char* pCurr);
static void extractLabel(ParsedLine* pObject, const SizedString* pLine, const char** ppCurr);
static int containsLabel(const SizedString* pLine, const char* pCurr);
static void findNextWhitespace(const SizedString* pLine, const char** ppCurr);
static void extractOperator(ParsedLine* pObject, const SizedString* pLine, const char** ppCurr);
static void extractOperands(ParsedLine* pObject, const SizedString* pLine, const char** ppCurr);
static void skipOverWhitespace(const SizedString* pLine, const char** ppCurr);
static int isEndOfLineOrComment(const SizedString* pLine, const char* pCurr);
static int isStartOfComment(const SizedString* pLine, const char* pCurr);
static void findEndOfLineOrComment(const SizedString* pLine, const char** ppCurr);
void ParseLine(ParsedLine* pObject, const SizedString* pLine)
{
    const char* pCurr;
    
    memset(pObject, 0, sizeof(*pObject));
    SizedString_EnumStart(pLine, &pCurr);
    if (isLineEmptyOrFullLineComment(pLine, pCurr))
        return;
    
    extractLabel(pObject, pLine, &pCurr);
    extractOperator(pObject, pLine, &pCurr);
    extractOperands(pObject, pLine, &pCurr);
}

static int isLineEmptyOrFullLineComment(const SizedString* pLine, const char* pCurr)
{
    return isLineEmpty(pLine, pCurr) || isFullLineComment(pLine, pCurr);
}

static int isLineEmpty(const SizedString* pLine, const char* pCurr)
{
    return SizedString_EnumCurr(pLine, pCurr) == '\0';
}

static int isFullLineComment(const SizedString* pLine, const char* pCurr)
{
    char currChar = SizedString_EnumCurr(pLine, pCurr);
    return currChar == '*' || currChar == ';';
}

static void extractLabel(ParsedLine* pObject, const SizedString* pLine, const char** ppCurr)
{
    if (containsLabel(pLine, *ppCurr))
    {
        const char* pLabel = *ppCurr;
        findNextWhitespace(pLine, ppCurr);
        pObject->label = SizedString_Init(pLabel, *ppCurr - pLabel);
    }
}

static int containsLabel(const SizedString* pLine, const char* pCurr)
{
    return !isspace(SizedString_EnumCurr(pLine, pCurr));
}

static void findNextWhitespace(const SizedString* pLine, const char** ppCurr)
{
    while (SizedString_EnumRemaining(pLine, *ppCurr) && !isspace(SizedString_EnumCurr(pLine, *ppCurr)))
        SizedString_EnumNext(pLine, ppCurr);
}

static void extractOperator(ParsedLine* pObject, const SizedString* pLine, const char** ppCurr)
{
    skipOverWhitespace(pLine, ppCurr);
    if (!isEndOfLineOrComment(pLine, *ppCurr))
    {
        const char* pStart = *ppCurr;
        findNextWhitespace(pLine, ppCurr);
        pObject->op = SizedString_Init(pStart, *ppCurr - pStart);
    }
}

static void skipOverWhitespace(const SizedString* pLine, const char** ppCurr)
{
    while (SizedString_EnumRemaining(pLine, *ppCurr) && isspace(SizedString_EnumCurr(pLine, *ppCurr)))
        SizedString_EnumNext(pLine, ppCurr);
}

static int isEndOfLineOrComment(const SizedString* pLine, const char* pCurr)
{
    return isLineEmpty(pLine, pCurr) || isStartOfComment(pLine, pCurr);
}

static int isStartOfComment(const SizedString* pLine, const char* pCurr)
{
    /* The `;' must either begin a line or be preceded by whitespace.
       Otherwise, we need to consider it as a delimiter for arguments to a
       macro.

       -- tkchia 20131018
    */
    return SizedString_EnumCurr(pLine, pCurr) == ';' &&
        (pCurr == pLine->pString || isspace((unsigned char)pCurr[-1]));
}

static void extractOperands(ParsedLine* pObject, const SizedString* pLine, const char** ppCurr)
{
    skipOverWhitespace(pLine, ppCurr);
    if (!isEndOfLineOrComment(pLine, *ppCurr))
    {
        const char* pStart = *ppCurr;
        findEndOfLineOrComment(pLine, ppCurr);
        pObject->operands = SizedString_Init(pStart, *ppCurr - pStart);
    }
}

static void findEndOfLineOrComment(const SizedString* pLine, const char** ppCurr)
{
    while (SizedString_EnumRemaining(pLine, *ppCurr) && !isEndOfLineOrComment(pLine, *ppCurr))
        SizedString_EnumNext(pLine, ppCurr);
}
