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


static int isLineEmptyOrFullLineComment(char* pLine);
static int isLineEmpty(char* pLine);
static int isFullLineComment(char* pLine);
static char* extractLabel(ParsedLine* pObject, char* pLine);
static int containsLabel(char* pLine);
static char* extractOperator(ParsedLine* pObject, char* pLine);
static char* extractField(char** ppField, char* pLine);
static char* skipOverWhitespace(char* pCurr);
static int isEndOfLineOrComment(char* pLine);
static int isStartOfComment(char* pLine);
static char* findNextWhitespace(char* pCurr);
static char* extractOperands(ParsedLine* pObject, char* pLine);
void ParseLine(ParsedLine* pObject, char* pLine)
{
    memset(pObject, 0, sizeof(*pObject));
    
    if (isLineEmptyOrFullLineComment(pLine))
        return;
    
    pLine = extractLabel(pObject, pLine);
    pLine = extractOperator(pObject, pLine);
    pLine = extractOperands(pObject, pLine);
}

static int isLineEmptyOrFullLineComment(char* pLine)
{
    return isLineEmpty(pLine) || isFullLineComment(pLine);
}

static int isLineEmpty(char* pLine)
{
    return pLine[0] == '\0';
}

static int isFullLineComment(char* pLine)
{
    return pLine[0] == '*' || pLine[0] == ';';
}

static char* extractLabel(ParsedLine* pObject, char* pLine)
{
    if (containsLabel(pLine))
    {
        pObject->pLabel = pLine;
        pLine = findNextWhitespace(pLine);
        if (*pLine)
            *pLine++ = '\0';
    }
    return pLine;
}

static int containsLabel(char* pLine)
{
    return !isspace(pLine[0]);
}

static char* extractOperator(ParsedLine* pObject, char* pLine)
{
    return extractField(&pObject->pOperator, pLine);
}

static char* extractOperands(ParsedLine* pObject, char* pLine)
{
    return extractField(&pObject->pOperands, pLine);
}

static char* extractField(char** ppField, char* pLine)
{
    pLine = skipOverWhitespace(pLine);
    if (!isEndOfLineOrComment(pLine))
    {
        *ppField = pLine;
        pLine = findNextWhitespace(pLine);
        if (*pLine)
            *pLine++ = '\0';
    }
    return pLine;
}

static char* skipOverWhitespace(char* pCurr)
{
    while (*pCurr && isspace(*pCurr))
        pCurr++;
    return pCurr;
}

static int isEndOfLineOrComment(char* pLine)
{
    return isLineEmpty(pLine) || isStartOfComment(pLine);
}

static int isStartOfComment(char* pLine)
{
    return *pLine == ';';
}

static char* findNextWhitespace(char* pCurr)
{
    while (*pCurr && !isspace(*pCurr))
        pCurr++;
    return pCurr;
}
