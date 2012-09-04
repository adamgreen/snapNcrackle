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
#include "SnapCommandLine.h"
#include "SnapCommandLineTest.h"

static void displayUsage(void)
{
    printf("Usage: snap [--list listFilename] sourceFilename\n\n"
           "Where: --list listFilename allows the list file for the assembly\n"
           "         process to be output to the specified file.  By defaul it\n"
           "         will be sent to stdout.\n"
           "       sourceFilename is the required name of an input assembly\n"
           "         language file.\n");
}


static int parseArgument(SnapCommandLine* pThis, int argc, const char** ppArgs);
static int hasDoubleDashPrefix(const char* pArgument);
static int parseFlagArgument(SnapCommandLine* pThis, int argc, const char** ppArgs);
static void parseListFilename(SnapCommandLine* pThis, int argc, const char* pFormat);
static int parseFilenameArgument(SnapCommandLine* pThis, int argc, const char* pArgument);
static void throwIfRequiredArgumentNotSpecified(SnapCommandLine* pThis);


__throws void SnapCommandLine_Init(SnapCommandLine* pThis, int argc, const char** argv)
{
    memset(pThis, 0, sizeof(*pThis));
    
    while (argc)
    {
        int argumentsUsed;
        
        __try
            argumentsUsed = parseArgument(pThis, argc, argv);
        __catch
            __rethrow;
            
        argc -= argumentsUsed;
        argv += argumentsUsed;
    }

    __try
        throwIfRequiredArgumentNotSpecified(pThis);
    __catch
        __rethrow;
}

static int parseArgument(SnapCommandLine* pThis, int argc, const char** ppArgs)
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

static int parseFlagArgument(SnapCommandLine* pThis, int argc, const char** ppArgs)
{
    if (0 == strcasecmp(*ppArgs, "--list"))
    {
        __try
            parseListFilename(pThis, argc - 1, ppArgs[1]);
        __catch
            __rethrow;

        return 2;
    }
    else
    {
        displayUsage();
        __throw(invalidArgumentException);
    }
}

static void parseListFilename(SnapCommandLine* pThis, int argc, const char* pListFilename)
{
    if (argc < 1)
    {
        displayUsage();
        __throw(invalidArgumentException);
    }
    pThis->pListFilename = pListFilename;
}

static int parseFilenameArgument(SnapCommandLine* pThis, int argc, const char* pArgument)
{
    if (!pThis->pSourceFilename)
    {
        pThis->pSourceFilename = pArgument;
        return 1;
    }
    else
    {
        displayUsage();
        __throw(invalidArgumentException);
    }
}

static void throwIfRequiredArgumentNotSpecified(SnapCommandLine* pThis)
{
    if (!pThis->pSourceFilename)
    {
        displayUsage();
        __throw(invalidArgumentException);
    }
}
