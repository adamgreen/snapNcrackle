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
#ifndef _PARSE_LINE_H_
#define _PARSE_LINE_H_

#include "SizedString.h"

typedef struct ParsedLine
{
    SizedString label;
    SizedString op;
    SizedString operands;
} ParsedLine;

void ParseLine(ParsedLine* pObject, const SizedString* pLine);

#endif /* _PARSE_LINE_H_ */
