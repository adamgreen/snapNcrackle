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
#include "ExpressionEval.h"
#include "ExpressionEvalTest.h"
#include "AssemblerPriv.h"


static int isHexValue(const char* pValue);
static int isImmediateValue(const char* pValue);
__throws Expression ExpressionEval(Assembler* pAssembler, const char* pOperands)
{
    Symbol*    pSymbol = NULL;
    Expression expression;
    
    memset(&expression, 0, sizeof(expression));
    
    if (isHexValue(pOperands))
    {
        expression.value = strtoul(&pOperands[1], NULL, 16);
        expression.type = TYPE_ABSOLUTE;
        return expression;
    }
    else if (isImmediateValue(pOperands))
    {
        expression = ExpressionEval(pAssembler, &pOperands[1]);
        expression.type = TYPE_IMMEDIATE;
        if (expression.value > 0xFF)
        {
            LOG_ERROR(pAssembler, "Immediate expression '%s' doesn't fit in 8-bits.", pOperands);
            __throw_and_return(invalidArgumentException, expression);
        }
        return expression;
    }
    else if (NULL != (pSymbol = Assembler_FindSymbol(pAssembler, pOperands)))
    {
        expression = pSymbol->expression;
        return expression;
    }
    else
    {
        LOG_ERROR(pAssembler, "Unexpected prefix in '%s' expression.", pOperands);
        __throw_and_return(invalidArgumentException, expression);
    }
}

static int isHexValue(const char* pValue)
{
    return *pValue == '$';
}

static int isImmediateValue(const char* pValue)
{
    return *pValue == '#';
}
