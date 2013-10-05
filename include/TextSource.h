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
#ifndef _TEXT_SOURCE_H_H_
#define _TEXT_SOURCE_H_H_

#include "SizedString.h"
#include "TextFile.h"

typedef struct TextSource TextSource;

int          TextSource_IsEndOfFile(TextSource* pThis);
SizedString  TextSource_GetNextLine(TextSource* pThis);
unsigned int TextSource_GetLineNumber(TextSource* pThis);
const char*  TextSource_GetFilename(TextSource* pThis);
const char*  TextSource_GetFirstFilename(TextSource* pThis);
TextFile*    TextSource_GetTextFile(TextSource* pThis);

void         TextSource_FreeAll(void);
void         TextSource_StackPush(TextSource** ppTopOfStack, TextSource* pToPush);
void         TextSource_StackPop(TextSource** ppTopOfStack);
unsigned int TextSource_StackDepth(TextSource* pTopOfStack);

#endif /* _TEXT_SOURCE_H_H_ */
