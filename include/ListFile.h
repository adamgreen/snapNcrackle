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
#ifndef _LIST_FILE_H_
#define _LIST_FILE_H_

#include <stdio.h>
#include "LineInfo.h"
#include "try_catch.h"


typedef struct ListFile ListFile;


__throws ListFile* ListFile_Create(FILE* pOutputFile);
         void      ListFile_Free(ListFile* pThis);
         
         void      ListFile_OutputLine(ListFile* pThis, LineInfo* pLineInfo);

#endif /* _LIST_FILE_H_ */
