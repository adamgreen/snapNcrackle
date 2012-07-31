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
#include "ParseCSV.h"
#include "ParseCSVTest.h"
#include "util.h"


struct ParseCSV
{
    char*   pLine;
    char**  ppFields;
    size_t  allocatedFieldCount;
    size_t  fieldCount;
};


__throws ParseCSV* ParseCSV_Create(void)
{
    return allocateAndZero(sizeof(ParseCSV));
}


void ParseCSV_Free(ParseCSV* pThis)
{
    if (!pThis)
        return;
        
    free(pThis->ppFields);
    free(pThis);
}


static size_t getFieldCount(char* pLine);
static size_t getCommaCount(char* pLine);
static void growFieldArrayIfNecessary(ParseCSV* pThis, size_t fieldCount);
static size_t fieldArraySizeForFieldsAndNullTerminator(size_t fieldCount);
static void setupFieldPointers(ParseCSV* pThis);
static char* findComma(char* pCurr);
__throws void ParseCSV_Parse(ParseCSV* pThis, char* pLine)
{
    size_t fieldCount = getFieldCount(pLine);
    __try
        growFieldArrayIfNecessary(pThis, fieldCount);
    __catch
        __rethrow;
    
    pThis->pLine = pLine;
    pThis->fieldCount = fieldCount;
    setupFieldPointers(pThis);
}

static size_t getFieldCount(char* pLine)
{
    if (!pLine)
        return 0;
    if (pLine[0] == '\0')
        return 0;
    return getCommaCount(pLine) + 1;
}

static size_t getCommaCount(char* pLine)
{
    size_t commaCount = 0;
    
    while (*pLine)
    {
        if (*pLine == ',')
            commaCount++;
        pLine++;
    }
    
    return commaCount;
}

static void growFieldArrayIfNecessary(ParseCSV* pThis, size_t fieldCount)
{
    size_t requiredArraySize = fieldArraySizeForFieldsAndNullTerminator(fieldCount);
    if (requiredArraySize > pThis->allocatedFieldCount)
    {
        char** ppRealloc = realloc(pThis->ppFields, requiredArraySize * sizeof(*ppRealloc));
        if (!ppRealloc)
            __throw(outOfMemoryException);
        pThis->ppFields = ppRealloc;
        pThis->allocatedFieldCount = requiredArraySize;
    }
}

static size_t fieldArraySizeForFieldsAndNullTerminator(size_t fieldCount)
{
    return fieldCount + 1;
}

static void setupFieldPointers(ParseCSV* pThis)
{
    char** ppField = pThis->ppFields;
    char*  pCurr = pThis->pLine;
    char*  pComma;
    
    if (pThis->pLine == NULL || *pThis->pLine == '\0')
    {
        *ppField = NULL;
        return;
    }
    
    for (;;)
    {
        *ppField++ = pCurr;
        pComma = findComma(pCurr);
        if (*pComma == '\0')
            break;
        *pComma = '\0';
        pCurr = pComma + 1;
    }
    
    *ppField = NULL;
}

static char* findComma(char* pCurr)
{
    while (*pCurr)
    {
        if (*pCurr == ',')
            return pCurr;
        pCurr++;
    }
    return pCurr;
}


size_t ParseCSV_FieldCount(ParseCSV* pThis)
{
    return pThis->fieldCount;
}


const char** ParseCSV_FieldPointers(ParseCSV* pThis)
{
    return (const char**)pThis->ppFields;
}
