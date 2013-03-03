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
#ifndef _CRACKLE_COMMANDLINE_H_
#define _CRACKLE_COMMANDLINE_H_

#include "try_catch.h"


typedef enum CrackleImageFormat
{
    FORMAT_UNKNOWN = 0,
    FORMAT_NIB_5_25,
    FORMAT_HDV_3_5
} CrackleImageFormat;


typedef struct CrackleCommandLine
{
    const char*        pScriptFilename;
    const char*        pOutputImageFilename;
    CrackleImageFormat imageFormat;
} CrackleCommandLine;


__throws CrackleCommandLine CrackleCommandLine_Init(int argc, const char** argv);

#endif /* _CRACKLE_COMMANDLINE_H_ */
