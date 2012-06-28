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
#include <stdio.h>
#include "CommandLine.h"
#include "Assembler.h"

int main(int argc, const char** argv)
{
    Assembler*  pAssembler = NULL;
    CommandLine commandLine;
    
    __try
    {
        __throwing_func( CommandLine_Init(&commandLine, argc-1, argv+1) );
        __throwing_func( pAssembler = Assembler_CreateFromCommandLine(&commandLine) );
        __throwing_func( Assembler_RunMultiple(pAssembler) );
    }
    __catch
    {
    }
    
    Assembler_Free(pAssembler);
    
    return 0;
}
