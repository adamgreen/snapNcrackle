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
#include <string.h>
#include <stdio.h>
#include "CommandLine.h"
#include "CommandLineTest.h"

static void displayUsage(void)
{
    printf("Usage: snap sourceFilename ...\n\n"
           "Where: sourceFilename is the name of an input assembly language file.\n"
           "       Note: You can provide more than one sourceFilename on the\n"
           "             command line.\n");
}

__throws void SnapCommandLine_Init(SnapCommandLine* pThis, int argc, const char** argv)
{
    memset(pThis, 0, sizeof(*pThis));
    
    if (!argc)
    {
        displayUsage();
        __throw(invalidArgumentException);
    }
        
    pThis->sourceFileCount = argc;
    pThis->pSourceFiles = argv;
}
