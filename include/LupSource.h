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
#ifndef _LUP_SOURCE_H_
#define _LUP_SOURCE_H_

#include "try_catch.h"
#include "TextSource.h"
#include "TextFile.h"

__throws TextSource* LupSource_Create(TextFile* pTextFile, unsigned short loopIterations);

#endif /* _LUP_SOURCE_H_ */
