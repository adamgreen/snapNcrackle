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
#include "CrackleCommandLine.h"
#include "DiskImage.h"


int main(int argc, const char** argv)
{
    int                returnValue = 0;
    DiskImage*         pDiskImage = NULL;
    CrackleCommandLine commandLine;

    __try
    {
        __throwing_func( commandLine = CrackleCommandLine_Init(argc-1, argv+1) );
        __throwing_func( pDiskImage = DiskImage_Create() );
        __throwing_func( DiskImage_ProcessScriptFile(pDiskImage, commandLine.pScriptFilename) );
        __throwing_func( DiskImage_WriteImage(pDiskImage, commandLine.pOutputImageFilename) );
        printf("%s image built successfully.\n", commandLine.pOutputImageFilename);
    }
    __catch
    {
        printf("%s image build failed.\n", commandLine.pOutputImageFilename);
        returnValue = 1;
    }
    
    DiskImage_Free(pDiskImage);
    
    return returnValue;
}
