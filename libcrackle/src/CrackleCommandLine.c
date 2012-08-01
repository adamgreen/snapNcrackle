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
#include "CrackleCommandLine.h"
#include "CrackleCommandLineTest.h"

static void displayUsage(void)
{
    printf("Usage: crackle scriptFilename outputImageFilename\n\n"
           "Where: scriptFilename is the name of the input script to be used\n"
           "       for placing data in the image file.  Each line should have\n"
           "       this format:\n"
           "         RWTS16|RWTS18,objectFilename,startOffset,length,track,sector\n\n"
           "       outputImageFilename is the name of the nibble based image to be\n"
           "       created by this tool.");
}

__throws CrackleCommandLine CrackleCommandLine_Init(int argc, const char** argv)
{
    CrackleCommandLine commandLine;
    memset(&commandLine, 0, sizeof(commandLine));
    
    if (argc != 2)
    {
        displayUsage();
        __throw_and_return(invalidArgumentException, commandLine);
    }
    commandLine.pScriptFilename = argv[0];
    commandLine.pOutputImageFilename = argv[1];

    return commandLine;
}
