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
#include "SnapCommandLine.h"
#include "SnapCommandLineTest.h"
#include "util.h"

static void displayUsage(void)
{
    printf("Usage: snap [--list listFilename] [--putdirs includeDir1;includeDir2...]\n"
           "            [--outdir outputDirectory]sourceFilename\n\n"
           "Where: --list listFilename allows the list file for the assembly\n"
           "         process to be output to the specified file.  By default it\n"
           "         will be sent to stdout.\n"
           "       --putdirs sets the directories (semi-colon separated) in which\n"
           "         files will be searched when including files with PUT directive.\n"
           "       --outdir sets the directory where output files from directives\n"
           "         like USR and SAV should be stored.\n"
           "       sourceFilename is the required name of an input assembly\n"
           "         language file.\n");
}


static int parseArgument(SnapCommandLine* pThis, int argc, const char** ppArgs);
static int hasDoubleDashPrefix(const char* pArgument);
static int parseFlagArgument(SnapCommandLine* pThis, int argc, const char** ppArgs);
static void parseStringParamter(const char** ppDestField, int argc, const char* pSourceArgument);
static int parseFilenameArgument(SnapCommandLine* pThis, int argc, const char* pArgument);
static void throwIfRequiredArgumentNotSpecified(SnapCommandLine* pThis);


__throws void SnapCommandLine_Init(SnapCommandLine* pThis, int argc, const char** argv)
{
    __try
    {
        memset(pThis, 0, sizeof(*pThis));
        while (argc)
        {
            int argumentsUsed = parseArgument(pThis, argc, argv);
            argc -= argumentsUsed;
            argv += argumentsUsed;
        }
        throwIfRequiredArgumentNotSpecified(pThis);
    }
    __catch
    {
        displayUsage();
        __rethrow;
    }
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
    struct
    {
        const char* pFlag;
        int         destStringOffsetInThis;
    } static const flagArguments[] =
    {
        { "--list",    offsetof(SnapCommandLine, assemblerInitParams) + offsetof(AssemblerInitParams, pListFilename) },
        { "--putdirs", offsetof(SnapCommandLine, assemblerInitParams) + offsetof(AssemblerInitParams, pPutDirectories) },
        { "--outdir",  offsetof(SnapCommandLine, assemblerInitParams) + offsetof(AssemblerInitParams, pOutputDirectory) }
    };
    size_t i;
    
    for (i = 0 ; i < ARRAYSIZE(flagArguments) ; i++)
    {
        if (0 == strcasecmp(*ppArgs, flagArguments[i].pFlag))
        {
            const char** ppDestField = (const char**)((char*)pThis + flagArguments[i].destStringOffsetInThis);
            parseStringParamter(ppDestField, argc - 1, ppArgs[1]);
            return 2;
        }
    }

    __throw(invalidArgumentException);
}

static void parseStringParamter(const char** ppDestField, int argc, const char* pSourceArgument)
{
    if (argc < 1)
        __throw(invalidArgumentException);

    *ppDestField = pSourceArgument;
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
        __throw(invalidArgumentException);
    }
}

static void throwIfRequiredArgumentNotSpecified(SnapCommandLine* pThis)
{
    if (!pThis->pSourceFilename)
        __throw(invalidArgumentException);
}
