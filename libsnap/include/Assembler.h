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
#ifndef _ASSEMBLER_H_
#define _ASSEMBLER_H_

// UNDONE: Remove CommandLine references.
#include "CommandLine.h"
#include "try_catch.h"


typedef struct Assembler Assembler;


__throws Assembler* Assembler_CreateFromString(char* pText);
__throws Assembler* Assembler_CreateFromFile(const char* pSourceFilename);
         void       Assembler_Free(Assembler* pThis);

         void       Assembler_Run(Assembler* pThis);

// UNDONE: Move this code out into a Builder or similar module later.
__throws Assembler* Assembler_CreateFromCommandLine(CommandLine* pCommandLine);
__throws void       Assembler_RunMultiple(Assembler* pThis);

#endif /* _ASSEMBLER_H_ */
