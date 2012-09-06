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
#include <ctype.h>
#include "ParseLine.h"


static int isLineEmptyOrFullLineComment(const char* pLine);
static int isLineEmpty(const char* pLine);
static int isFullLineComment(const char* pLine);
static const char* extractLabel(ParsedLine* pObject, const char* pLine);
static int containsLabel(const char* pLine);
static const char* extractOperator(ParsedLine* pObject, const char* pLine);
static const char* extractField(SizedString* pField, const char* pLine);
static const char* skipOverWhitespace(const char* pCurr);
static int isEndOfLineOrComment(const char* pLine);
static int isStartOfComment(const char* pLine);
static const char* findNextWhitespace(const char* pCurr);
static const char* extractOperands(ParsedLine* pObject, const char* pLine);
void ParseLine(ParsedLine* pObject, const char* pLine)
{
    memset(pObject, 0, sizeof(*pObject));
    
    if (isLineEmptyOrFullLineComment(pLine))
        return;
    
    pLine = extractLabel(pObject, pLine);
    pLine = extractOperator(pObject, pLine);
    pLine = extractOperands(pObject, pLine);
}

static int isLineEmptyOrFullLineComment(const char* pLine)
{
    return isLineEmpty(pLine) || isFullLineComment(pLine);
}

static int isLineEmpty(const char* pLine)
{
    return pLine[0] == '\0';
}

static int isFullLineComment(const char* pLine)
{
    return pLine[0] == '*' || pLine[0] == ';';
}

static const char* extractLabel(ParsedLine* pObject, const char* pLine)
{
    if (containsLabel(pLine))
    {
        const char* pLabel = pLine;
        pLine = findNextWhitespace(pLine);
        pObject->label = SizedString_Init(pLabel, pLine - pLabel);
    }
    return pLine;
}

static int containsLabel(const char* pLine)
{
    return !isspace(pLine[0]);
}

static const char* extractOperator(ParsedLine* pObject, const char* pLine)
{
    return extractField(&pObject->op, pLine);
}

static const char* extractOperands(ParsedLine* pObject, const char* pLine)
{
    return extractField(&pObject->operands, pLine);
}

static const char* extractField(SizedString* pField, const char* pLine)
{
    pLine = skipOverWhitespace(pLine);
    if (!isEndOfLineOrComment(pLine))
    {
        const char* pStart = pLine;
        pLine = findNextWhitespace(pLine);
        *pField = SizedString_Init(pStart, pLine - pStart);
    }
    return pLine;
}

static const char* skipOverWhitespace(const char* pCurr)
{
    while (*pCurr && isspace(*pCurr))
        pCurr++;
    return pCurr;
}

static int isEndOfLineOrComment(const char* pLine)
{
    return isLineEmpty(pLine) || isStartOfComment(pLine);
}

static int isStartOfComment(const char* pLine)
{
    return *pLine == ';';
}

static const char* findNextWhitespace(const char* pCurr)
{
    while (*pCurr && !isspace(*pCurr))
        pCurr++;
    return pCurr;
}
