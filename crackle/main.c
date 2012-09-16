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
#include <string.h>
#include "CrackleCommandLine.h"
#include "NibbleDiskImage.h"
#include "BlockDiskImage.h"


static DiskImage* allocateDiskImageObject(CrackleCommandLine* pCommandLine);
int main(int argc, const char** argv)
{
    int                returnValue = 0;
    DiskImage*         pDiskImage = NULL;
    CrackleCommandLine commandLine;

    memset(&commandLine, 0, sizeof(commandLine));
    __try
    {
        commandLine = CrackleCommandLine_Init(argc-1, argv+1);
        pDiskImage = allocateDiskImageObject(&commandLine);
        DiskImage_ProcessScriptFile(pDiskImage, commandLine.pScriptFilename);
        DiskImage_WriteImage(pDiskImage, commandLine.pOutputImageFilename);
        printf("%s image built successfully.\n", commandLine.pOutputImageFilename);
    }
    __catch
    {
        printf("%s image build failed.\n", commandLine.pOutputImageFilename ? commandLine.pOutputImageFilename : "");
        returnValue = 1;
    }
    
    DiskImage_Free(pDiskImage);
    
    return returnValue;
}

static DiskImage* allocateDiskImageObject(CrackleCommandLine* pCommandLine)
{
    if (pCommandLine->imageFormat == FORMAT_NIB_5_25)
        return (DiskImage*) NibbleDiskImage_Create();
    else if (pCommandLine->imageFormat == FORMAT_2MG_3_5)
        return (DiskImage*) BlockDiskImage_Create(BLOCK_DISK_IMAGE_3_5_BLOCK_COUNT);
    else
        return NULL;
}
