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
#include "AddressingMode.h"
#include "AddressingModeTest.h"
#include "SizedString.h"

static int usesImpliedAddressing(const char* pOperands);
static int usesImmediateAddressing(const char* pOperands);
AddressingMode AddressingMode_Eval(Assembler* pAssembler, const char* pOperands)
{
    AddressingMode addressingMode;
    SizedString    operandsString = SizedString_InitFromString(pOperands);
    const char*    pComma = SizedString_strchr(&operandsString, ',');
#ifdef UNDONE
    const char*    pOpenParen;
    const char*    pCloseParen;
#endif /* UNDONE */

    memset(&addressingMode, 0, sizeof(addressingMode));
    if (usesImpliedAddressing(pOperands))
    {
        addressingMode.mode = ADDRESSING_MODE_IMPLIED;
        return addressingMode;
    }
    else if (pComma)
    {
        SizedString beforeComma;
        SizedString afterComma;
        
        SizedString_SplitString(&operandsString, ',', &beforeComma, &afterComma);
        if (0 == SizedString_strcasecmp(&afterComma, "X"))
        {
            addressingMode.mode = ADDRESSING_MODE_ABSOLUTE_INDEXED_X;
            addressingMode.expression = ExpressionEvalSizedString(pAssembler, &beforeComma);
            return addressingMode;
        }
        else if (0 == SizedString_strcasecmp(&afterComma, "Y"))
        {
            addressingMode.mode = ADDRESSING_MODE_ABSOLUTE_INDEXED_Y;
            addressingMode.expression = ExpressionEvalSizedString(pAssembler, &beforeComma);
            return addressingMode;
        }
    }
    else if (usesImmediateAddressing(pOperands))
    {
        addressingMode.mode = ADDRESSING_MODE_IMMEDIATE;
        addressingMode.expression = ExpressionEval(pAssembler, pOperands);
        return addressingMode;
    }
    else
    {
        addressingMode.mode = ADDRESSING_MODE_ABSOLUTE;
        addressingMode.expression = ExpressionEval(pAssembler, pOperands);
        return addressingMode;
    }
    
    addressingMode.mode = ADDRESSING_MODE_INVALID;
    return addressingMode;
}

static int usesImpliedAddressing(const char* pOperands)
{
    return pOperands == NULL;
}

static int usesImmediateAddressing(const char* pOperands)
{
    return pOperands[0] == '#';
}
