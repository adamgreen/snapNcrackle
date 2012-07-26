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
#ifndef _ADDRESSING_MODE_H_
#define _ADDRESSING_MODE_H_

#include "Assembler.h"
#include "ExpressionEval.h"

typedef enum AddressingModes
{
    ADDRESSING_MODE_ABSOLUTE = 0,
    ADDRESSING_MODE_IMMEDIATE,
    ADDRESSING_MODE_IMPLIED,
    ADDRESSING_MODE_ABSOLUTE_INDEXED_X,
    ADDRESSING_MODE_ABSOLUTE_INDEXED_Y,
    ADDRESSING_MODE_INDEXED_INDIRECT,
    ADDRESSING_MODE_INDIRECT_INDEXED,
    ADDRESSING_MODE_INDIRECT,
    ADDRESSING_MODE_INVALID
} AddressingModes;

typedef struct AddressingMode
{
    Expression      expression;
    AddressingModes mode;
} AddressingMode;


__throws AddressingMode AddressingMode_Eval(Assembler* pAssembler, const char* pOperands);

#endif /* _ADDRESSING_MODE_H_ */
