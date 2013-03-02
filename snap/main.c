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
#include "SnapCommandLine.h"
#include "Assembler.h"
#include "util.h"

static void displayErrorCountIfAnyWereEncountered(Assembler* pAssembler);
int main(int argc, const char** argv)
{
    int                 returnValue = 0;
    Assembler*          pAssembler = NULL;
    SnapCommandLine     commandLine;

    __try
    {
        SnapCommandLine_Init(&commandLine, argc-1, argv+1);
        pAssembler = Assembler_CreateFromFile(commandLine.pSourceFilename, &commandLine.assemblerInitParams);
        Assembler_Run(pAssembler);
        displayErrorCountIfAnyWereEncountered(pAssembler);
    }
    __catch
    {
        returnValue = 1;
    }
    
    Assembler_Free(pAssembler);
    
    return returnValue;
}

static void displayErrorCountIfAnyWereEncountered(Assembler* pAssembler)
{
    unsigned int errorCount = Assembler_GetErrorCount(pAssembler);
    unsigned int warningCount = Assembler_GetWarningCount(pAssembler);
    
    if (errorCount || warningCount)
        printf(LINE_ENDING "Encountered %d %s and %d %s during assembly." LINE_ENDING,
               errorCount, errorCount != 1 ? "errors" : "error",
               warningCount, warningCount != 1 ? "warnings" : "warning");
    else
        printf(LINE_ENDING "Assembly completed successfully." LINE_ENDING);
}
