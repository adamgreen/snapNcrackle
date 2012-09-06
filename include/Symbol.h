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
#ifndef _SYMBOL_H_
#define _SYMBOL_H_

#include "ExpressionEval.h"
#include "SizedString.h"
#include "LineInfo.h"


typedef struct LineInfo LineInfo;
typedef struct SymbolLineReference SymbolLineReference;


struct Symbol
{
    SymbolLineReference* pLineReferences;
    SymbolLineReference* pEnumLineReference;
    LineInfo*            pDefinedLine;
    struct Symbol*       pNext;
    SizedString          globalKey;
    SizedString          localKey;
    Expression           expression;
    unsigned int         flags;
};


#endif /* _SYMBOL_H_ */
