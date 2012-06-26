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


static int isLineEmpty(char* pLine);
static int isFullLineComment(char* pLine);
static int containsLabel(char* pLine);
static int isEndOfLineOrComment(char* pLine);
static int isStartOfComment(char* pLine);
static char* findNextWhitespace(char* pCurr);
static char* skipOverWhitespace(char* pCurr);
void ParseLine(ParsedLine* pObject, char* pLine)
{
    memset(pObject, 0, sizeof(*pObject));
    
    if (isLineEmpty(pLine) || isFullLineComment(pLine))
        return;
    
    if (containsLabel(pLine))
    {
        pObject->pLabel = pLine;
        pLine = findNextWhitespace(pLine);
        if (*pLine)
            *pLine++ = '\0';
    }
    
    pLine = skipOverWhitespace(pLine);
    if (!isEndOfLineOrComment(pLine))
    {
        pObject->pOperator = pLine;
        pLine = findNextWhitespace(pLine);
        if (*pLine)
            *pLine++ = '\0';
    }

    pLine = skipOverWhitespace(pLine);
    if (!isEndOfLineOrComment(pLine))
    {
        pObject->pOperands = pLine;
        pLine = findNextWhitespace(pLine);
        if (*pLine)
            *pLine++ = '\0';
    }
}

static int isLineEmpty(char* pLine)
{
    return pLine[0] == '\0';
}

static int isFullLineComment(char* pLine)
{
    return pLine[0] == '*';
}

static int containsLabel(char* pLine)
{
    return !isspace(pLine[0]);
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

static char* skipOverWhitespace(char* pCurr)
{
    while (*pCurr && isspace(*pCurr))
        pCurr++;
    return pCurr;
}
