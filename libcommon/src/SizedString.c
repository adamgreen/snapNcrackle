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
#include "SizedString.h"
#include "SizedStringTest.h"


SizedString SizedString_Init(const char* pString, size_t stringLength)
{
    SizedString sizedString;
    
    sizedString.pString = pString;
    sizedString.stringLength = stringLength;
    
    return sizedString;
}


SizedString SizedString_InitFromString(const char* pString)
{
    return SizedString_Init(pString, pString ? strlen(pString) : 0);
}


const char* SizedString_strchr(const SizedString* pString, char searchChar)
{
    size_t      charsLeft = pString->stringLength;
    const char* pCurr = pString->pString;

    while (charsLeft-- > 0)
    {
        if (*pCurr == searchChar)
            return pCurr;
        pCurr++;
    }

    return NULL;
}


const char* SizedString_strrchr(const SizedString* pString, char searchChar)
{
    const char* pStartOfString = pString->pString;
    const char* pCurr = pStartOfString + pString->stringLength - 1;

    while (pCurr >= pStartOfString)
    {
        if (*pCurr == searchChar)
            return pCurr;
        pCurr--;
    }

    return NULL;
}


int SizedString_strcmp(const SizedString* pString, const char* pSearchString)
{
    static const char  terminator = '\0';
    size_t             charsLeft = pString->stringLength;
    const char*        p1 = pString->pString;
    const char*        p2 = pSearchString;
    
    while (charsLeft > 0 && *p2)
    {
        int diff = *p1++ - *p2++;
        if (diff != 0)
            return diff;
        charsLeft--;
    }
    
    if (charsLeft == 0)
        p1 = &terminator;
    return *p1 - *p2;
}


int SizedString_strcasecmp(const SizedString* pString, const char* pSearchString)
{
    static const char  terminator = '\0';
    size_t             charsLeft = pString->stringLength;
    const char*        p1 = pString->pString;
    const char*        p2 = pSearchString;
    
    while (charsLeft > 0 && *p2)
    {
        int diff = tolower(*p1++) - tolower(*p2++);
        if (diff != 0)
            return diff;
        charsLeft--;
    }
    
    if (charsLeft == 0)
        p1 = &terminator;
    return tolower(*p1) - tolower(*p2);
}


int SizedString_Compare(const SizedString* pString1, const SizedString* pString2)
{
    static const char  terminator = '\0';
    size_t             charsLeft1 = pString1->stringLength;
    size_t             charsLeft2 = pString2->stringLength;
    const char*        p1 = pString1->pString;
    const char*        p2 = pString2->pString;
    
    while (charsLeft1 > 0 && charsLeft2 > 0)
    {
        int diff = *p1++ - *p2++;
        if (diff != 0)
            return diff;
        charsLeft1--;
        charsLeft2--;
    }
    
    if (charsLeft1 == 0)
        p1 = &terminator;
    if (charsLeft2 == 0)
        p2 = &terminator;
        
    return *p1 - *p2;
}


size_t SizedString_strlen(const SizedString* pString)
{
    return pString->stringLength;
}


__throws char* SizedString_strdup(const SizedString* pStringToCopy)
{
    char* pCopiedString;
    
    pCopiedString = malloc(pStringToCopy->stringLength + 1);
    if (!pCopiedString)
        __throw(outOfMemoryException);
    memcpy(pCopiedString, pStringToCopy->pString, pStringToCopy->stringLength + 1);
    
    return pCopiedString;
}


void SizedString_SplitString(const SizedString* pInput, char splitAtChar, SizedString* pBefore, SizedString* pAfter)
{
    const char* pSplitChar;
    
    *pBefore = *pInput;
    memset(pAfter, 0, sizeof(*pAfter));
    
    pSplitChar = SizedString_strchr(pInput, splitAtChar);
    if (!pSplitChar)
        return;
        
    pBefore->stringLength = pSplitChar - pInput->pString;
    pAfter->pString = pSplitChar + 1;
    pAfter->stringLength = pInput->stringLength - pBefore->stringLength - 1;
}


void SizedString_EnumStart(const SizedString* pString, const char** ppEnumerator)
{
    *ppEnumerator = pString->pString;
}


char SizedString_EnumCurr(const SizedString* pString, const char* pEnumerator)
{
    if ((size_t)(pEnumerator - pString->pString) >= pString->stringLength)
        return '\0';
    return *pEnumerator;
}


char SizedString_EnumNext(const SizedString* pString, const char** ppEnumerator)
{
    char ReturnValue = SizedString_EnumCurr(pString, *ppEnumerator);

    if (ReturnValue != '\0')
        (*ppEnumerator)++;
    
    return ReturnValue;
}


size_t SizedString_EnumRemaining(const SizedString* pString, const char* pEnumerator)
{
    int charsEnumerated = pEnumerator - pString->pString;
    
    if (charsEnumerated < 0)
        return 0;
    return pString->stringLength - (size_t)charsEnumerated;
}
