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
#ifndef _SIZED_STRING_H_
#define _SIZED_STRING_H_

typedef struct SizedString
{
    const char* pString;
    size_t      stringLength;
} SizedString;


SizedString SizedString_Init(const char* pString, size_t stringLength);
SizedString SizedString_InitFromString(const char* pString);

const char* SizedString_strchr(const SizedString* pString, char searchChar);
int         SizedString_strcmp(const SizedString* pString, const char* pSearchString);
int         SizedString_strcasecmp(const SizedString* pString, const char* pSearchString);
int         SizedString_Compare(const SizedString* pString1, const SizedString* pString2);
void        SizedString_SplitString(const SizedString* pInput, char splitAtChar, SizedString* pBefore, SizedString* pAfter);
size_t      SizedString_strlen(const SizedString* pString);

void        SizedString_EnumStart(const SizedString* pString, const char** ppEnumerator);
char        SizedString_EnumCurr(const SizedString* pString, const char* pEnumerator);
char        SizedString_EnumNext(const SizedString* pString, const char** ppEnumerator);
size_t      SizedString_EnumRemaining(const SizedString* pString, const char* pEnumerator);

#endif /* _SIZED_STRING_H_ */
