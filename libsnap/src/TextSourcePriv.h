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
#ifndef _TEXT_SOURCE_PRIV_H_
#define _TEXT_SOURCE_PRIV_H_

#include "TextSource.h"

typedef struct TextSourceVTable
{
    void         (*freeObject)(void *pThis);
    SizedString  (*getNextLine)(void* pThis);
    int          (*isEndOfFile)(void* pThis);
    unsigned int (*getLineNumber)(void* pThis);
    const char*  (*getFilename)(void* pThis);
} TextSourceVTable;


struct TextSource
{
    TextSourceVTable*   pVTable;
    struct TextSource*  pFreeNext;
    struct TextSource*  pStackPrev;
    TextFile*           pTextFile;
    unsigned int        stackDepth;
};

void TextSource_AddToFreeList(TextSource* pObjectToAdd);
void TextSource_SetTextFile(TextSource* pTextSource, TextFile* pTextFile);

#endif /* _TEXT_SOURCE_PRIV_H_ */
