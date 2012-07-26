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
#ifndef _LINE_BUFFER_H_
#define _LINE_BUFFER_H_

#include "try_catch.h"

typedef struct LineBuffer LineBuffer;


__throws LineBuffer* LineBuffer_Create(void);
         void        LineBuffer_Free(LineBuffer* pThis);

__throws void        LineBuffer_Set(LineBuffer* pThis, const char* pString);
         char*       LineBuffer_Get(LineBuffer* pThis);
         
#endif /* _LINE_BUFFER_H_ */
