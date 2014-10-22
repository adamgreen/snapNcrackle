/*  Copyright (C) 2014  Tee-Kiah Chia

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
#ifndef _MACRO_EXPANSION_SOURCE_H_
#define _MACRO_EXPANSION_SOURCE_H_

#include "try_catch.h"
#include "TextSource.h"
#include "TextFile.h"

__throws TextSource* MacroExpansionSource_Create(TextFile* pTextFile,
  unsigned int startingSourceLine, SizedString* macroExpansionLines,
  unsigned short numberOfLines);

#endif /* _LUP_SOURCE_H_ */
