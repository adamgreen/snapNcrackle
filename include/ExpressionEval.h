/*  Copyright (C) 2012  Adam Green (https://github.com/adamgreen)
    Copyright (C) 2013  Tee-Kiah Chia

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
#ifndef _EXPRESSION_EVAL_H_
#define _EXPRESSION_EVAL_H_

#include <stdint.h>
#include "try_catch.h"
#include "Assembler.h"
#include "SizedString.h"


#define EXPRESSION_FLAG_FORWARD_REFERENCE 1


typedef enum ExpressionType
{
    TYPE_ABSOLUTE = 0,
    TYPE_ZEROPAGE,
    TYPE_IMMEDIATE
} ExpressionType;

typedef struct Expression
{
    ExpressionType type;
    unsigned int   flags;
    uint32_t       value;
} Expression;


__throws Expression ExpressionEval(Assembler* pAssembler, SizedString* pOperands);
         Expression ExpressionEval_CreateAbsoluteExpression(uint32_t value);

#endif /* _EXPRESSION_EVAL_H_ */
