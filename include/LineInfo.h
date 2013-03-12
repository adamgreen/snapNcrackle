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
#ifndef _LINE_INFO_H_
#define _LINE_INFO_H_

#include "Symbol.h"
#include "TextSource.h"

#define LINEINFO_FLAG_WAS_EQU           1
#define LINEINFO_FLAG_FORWARD_REFERENCE 2


typedef struct Symbol Symbol;

typedef enum InstructionSetSupported
{
    INSTRUCTION_SET_6502 = 0,
    INSTRUCTION_SET_65C02,
    INSTRUCTION_SET_65816,
    INSTRUCTION_SET_INVALID
} InstructionSetSupported;


struct LineInfo
{
    SizedString             lineText;
    Symbol*                 pSymbol;
    TextSource*             pTextSource;
    struct LineInfo*        pNext;
    unsigned char*          pMachineCode;
    size_t                  machineCodeSize;
    InstructionSetSupported instructionSet;
    int                     indentation;
    unsigned int            lineNumber;
    unsigned int            flags;
    unsigned short          address;
    unsigned short          equValue;
};

#endif /* _LINE_INFO_H_ */
