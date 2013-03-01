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
#include <string.h>
#include <stdio.h>
#include "CrackleCommandLine.h"
#include "CrackleCommandLineTest.h"
#include "version.h"

static void displayCopyrightNotice(void)
{
    printf("crackle - Simple Disk Imaging Tool for Apple II (" VERSION_STRING ")\n\n"
           COPYRIGHT_NOTICE
           "\n");
}

static void displayUsage(void)
{
    printf("Usage: crackle --format image_format scriptFilename outputImageFilename\n\n"
           "Where: --format image_format indicates the type outputImage is to be\n"
           "         created.  image_format can be one of:\n"
           "           nib_5.25 - creates a .nib nibble image for a 5 1/4\" disk.\n"
           "           hdv_3.5 - creates a .HDV block image for a 3 1/2\" disk.\n"
           "       scriptFilename is the name of the input script to be used\n"
           "         for placing data in the image file.  Each line should meet\n"
           "         one of these formats:\n"
           "           BLOCK,objectFilename,startOffset,length,block,[intraBlockOffset]\n"
           "           RWTS16,objectFilename,startOffset,length,track,sector\n"
           "           RWTS18,objectFilename,startOffset,length,side,track,sector,intraSectorOffset\n"
           "       outputImageFilename is the name of the image to be created by\n"
           "           this tool.\n\n");
}


static int parseArgument(CrackleCommandLine* pThis, int argc, const char** ppArgs);
static int hasDoubleDashPrefix(const char* pArgument);
static int parseFlagArgument(CrackleCommandLine* pThis, int argc, const char** ppArgs);
static void parseFormat(CrackleCommandLine* pThis, int argc, const char* pFormat);
static int parseFilenameArgument(CrackleCommandLine* pThis, int argc, const char* pArgument);
static void throwIfRequiredArgumentNotSpecified(CrackleCommandLine* pThis);


__throws CrackleCommandLine CrackleCommandLine_Init(int argc, const char** argv)
{
    CrackleCommandLine commandLine;
    memset(&commandLine, 0, sizeof(commandLine));
    
    __try
    {
        while (argc)
        {
            int argumentsUsed = parseArgument(&commandLine, argc, argv);

            argc -= argumentsUsed;
            argv += argumentsUsed;
        }
        throwIfRequiredArgumentNotSpecified(&commandLine);
    }
    __catch
    {
        displayCopyrightNotice();
        displayUsage();
        __rethrow;
    }
    
    return commandLine;
}

static int parseArgument(CrackleCommandLine* pThis, int argc, const char** ppArgs)
{
    if (hasDoubleDashPrefix(*ppArgs))
        return parseFlagArgument(pThis, argc, ppArgs);
    else
        return parseFilenameArgument(pThis, argc, *ppArgs);
}

static int hasDoubleDashPrefix(const char* pArgument)
{
    return pArgument[0] == '-' && pArgument[1] == '-';
}

static int parseFlagArgument(CrackleCommandLine* pThis, int argc, const char** ppArgs)
{
    if (0 == strcasecmp(*ppArgs, "--format"))
    {
        parseFormat(pThis, argc - 1, ppArgs[1]);
        return 2;
    }
    else
    {
        __throw(invalidArgumentException);
    }
}

static void parseFormat(CrackleCommandLine* pThis, int argc, const char* pFormat)
{
    if (argc < 1)
        __throw(invalidArgumentException);
    if (0 == strcasecmp(pFormat, "nib_5.25"))
        pThis->imageFormat = FORMAT_NIB_5_25;
    else if (0 == strcasecmp(pFormat, "hdv_3.5"))
        pThis->imageFormat = FORMAT_HDV_3_5;
    else
        __throw(invalidArgumentException);
}

static int parseFilenameArgument(CrackleCommandLine* pThis, int argc, const char* pArgument)
{
    if (!pThis->pScriptFilename)
    {
        pThis->pScriptFilename = pArgument;
        return 1;
    }
    else if (!pThis->pOutputImageFilename)
    {
        pThis->pOutputImageFilename = pArgument;
        return 1;
    }
    else
    {
        __throw(invalidArgumentException);
    }
}

static void throwIfRequiredArgumentNotSpecified(CrackleCommandLine* pThis)
{
    if (!pThis->pScriptFilename || !pThis->pOutputImageFilename || pThis->imageFormat == FORMAT_UNKNOWN)
        __throw(invalidArgumentException);
}
