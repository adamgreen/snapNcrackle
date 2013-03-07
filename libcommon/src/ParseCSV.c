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
#include "ParseCSV.h"
#include "ParseCSVTest.h"
#include "util.h"


struct ParseCSV
{
    SizedString* pFields;
    size_t       allocatedFieldCount;
    size_t       fieldCount;
    char         separator;
};


__throws ParseCSV* ParseCSV_Create(void)
{
    return ParseCSV_CreateWithCustomSeparator(',');
}


__throws ParseCSV* ParseCSV_CreateWithCustomSeparator(char separator)
{
    ParseCSV* pThis = allocateAndZero(sizeof(ParseCSV));
    pThis->separator = separator;
    return pThis;
}


void ParseCSV_Free(ParseCSV* pThis)
{
    if (!pThis)
        return;
        
    free(pThis->pFields);
    free(pThis);
}


static size_t getFieldCount(ParseCSV* pThis, const SizedString* pLine);
static int isStringEmptyOrNull(const SizedString* pLine);
static size_t getCharCount(const SizedString* pLine, char charToCount);
static void growFieldArrayIfNecessary(ParseCSV* pThis, size_t fieldCount);
static size_t fieldArraySizeForFieldsAndNullTerminator(size_t fieldCount);
static void setupFieldPointers(ParseCSV* pThis, const SizedString* pLine);
static const char* findChar(const SizedString* pLine, const char* pCurr, char separator);
__throws void ParseCSV_Parse(ParseCSV* pThis, const SizedString* pLine)
{
    size_t fieldCount = getFieldCount(pThis, pLine);
    growFieldArrayIfNecessary(pThis, fieldCount);
    pThis->fieldCount = fieldCount;
    setupFieldPointers(pThis, pLine);
}

static size_t getFieldCount(ParseCSV* pThis, const SizedString* pLine)
{
    if (isStringEmptyOrNull(pLine))
        return 0;
    return getCharCount(pLine, pThis->separator) + 1;
}

static int isStringEmptyOrNull(const SizedString* pLine)
{
    return !pLine || !pLine->pString || pLine->stringLength == 0;
}

static size_t getCharCount(const SizedString* pLine, char charToCount)
{
    const char* pEnum;
    size_t count = 0;

    SizedString_EnumStart(pLine, &pEnum);
    while (SizedString_EnumRemaining(pLine, pEnum))
    {
        if (SizedString_EnumNext(pLine, &pEnum) == charToCount)
            count++;
    }
    
    return count;
}

static void growFieldArrayIfNecessary(ParseCSV* pThis, size_t fieldCount)
{
    size_t requiredArraySize = fieldArraySizeForFieldsAndNullTerminator(fieldCount);
    if (requiredArraySize > pThis->allocatedFieldCount)
    {
        SizedString* pRealloc = realloc(pThis->pFields, requiredArraySize * sizeof(*pRealloc));
        if (!pRealloc)
            __throw(outOfMemoryException);
        pThis->pFields = pRealloc;
        pThis->allocatedFieldCount = requiredArraySize;
    }
}

static size_t fieldArraySizeForFieldsAndNullTerminator(size_t fieldCount)
{
    return fieldCount + 1;
}

static void setupFieldPointers(ParseCSV* pThis, const SizedString* pLine)
{
    SizedString* pField = pThis->pFields;
    const char* pCurr;
    const char* pStart;
    const char* pSeparator;
    
    if (isStringEmptyOrNull(pLine))
    {
        *pField = SizedString_InitFromString(NULL);
        return;
    }

    SizedString_EnumStart(pLine, &pCurr);
    for (;;)
    {
        pStart = pCurr;
        pSeparator = findChar(pLine, pCurr, pThis->separator);
        *pField++ = SizedString_Init(pStart, pSeparator - pStart);
        pCurr = pSeparator;
        if (SizedString_EnumNext(pLine, &pCurr) == '\0')
            break;
    }
    *pField = SizedString_InitFromString(NULL);
}

static const char* findChar(const SizedString* pLine, const char* pCurr, char separator)
{
    while (SizedString_EnumRemaining(pLine, pCurr))
    {
        const char* pPrev = pCurr;
        if (SizedString_EnumNext(pLine, &pCurr) == separator)
            return pPrev;
    }
    return pCurr;
}


size_t ParseCSV_FieldCount(ParseCSV* pThis)
{
    return pThis->fieldCount;
}


const SizedString* ParseCSV_FieldPointers(ParseCSV* pThis)
{
    return pThis->pFields;
}
