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
#include "AssemblerPriv.h"


static int usesImpliedAddressing(const char* pOperands);
static int usesImmediateAddressing(const char* pOperands);
__throws AddressingMode AddressingMode_Eval(Assembler* pAssembler, const char* pOperands)
{
    AddressingMode addressingMode;
    SizedString    operandsString = SizedString_InitFromString(pOperands);
    const char*    pComma = SizedString_strchr(&operandsString, ',');
    const char*    pOpenParen = SizedString_strchr(&operandsString, '(');
    const char*    pCloseParen = SizedString_strchr(&operandsString, ')');

    memset(&addressingMode, 0, sizeof(addressingMode));
    addressingMode.mode = ADDRESSING_MODE_INVALID;
    if (usesImpliedAddressing(pOperands))
    {
        addressingMode.mode = ADDRESSING_MODE_IMPLIED;
        return addressingMode;
    }
    else if (pOpenParen && pCloseParen && pComma && 
             pOpenParen == operandsString.pString && pComma > pOpenParen && pCloseParen > pComma)
    {
        SizedString beforeOpenParen;
        SizedString afterOpenParen;
        SizedString beforeComma;
        SizedString afterComma;

        SizedString_SplitString(&operandsString, '(', &beforeOpenParen, &afterOpenParen);
        SizedString_SplitString(&afterOpenParen, ',', &beforeComma, &afterComma);
        if (0 == SizedString_strcasecmp(&afterComma, "X)"))
        {
            __try
                addressingMode.expression = ExpressionEvalSizedString(pAssembler, &beforeComma);
            __catch
                __rethrow_and_return(addressingMode);

            addressingMode.mode = ADDRESSING_MODE_INDEXED_INDIRECT;
            return addressingMode;
        }
    }
    else if (pOpenParen && pCloseParen && pComma && 
             pOpenParen == operandsString.pString && pComma > pOpenParen && pComma > pCloseParen)
    {
        SizedString beforeOpenParen;
        SizedString afterOpenParen;
        SizedString beforeCloseParen;
        SizedString afterCloseParen;

        SizedString_SplitString(&operandsString, '(', &beforeOpenParen, &afterOpenParen);
        SizedString_SplitString(&afterOpenParen, ')', &beforeCloseParen, &afterCloseParen);
        if (0 == SizedString_strcasecmp(&afterCloseParen, ",Y"))
        {
            __try
                addressingMode.expression = ExpressionEvalSizedString(pAssembler, &beforeCloseParen);
            __catch
                __rethrow_and_return(addressingMode);
                
            if (addressingMode.expression.type != TYPE_ZEROPAGE)
            {
                LOG_ERROR(pAssembler, "'%.*s' isn't in page zero as required for indirect indexed addressing.", 
                          beforeCloseParen.stringLength, beforeCloseParen.pString);
                __throw_and_return(invalidArgumentException, addressingMode);
            }

            addressingMode.mode = ADDRESSING_MODE_INDIRECT_INDEXED;
            return addressingMode;
        }
    }
    else if (pOpenParen && pCloseParen && !pComma && pOpenParen < pCloseParen)
    {
        SizedString beforeOpenParen;
        SizedString afterOpenParen;
        SizedString beforeCloseParen;
        SizedString afterCloseParen;

        SizedString_SplitString(&operandsString, '(', &beforeOpenParen, &afterOpenParen);
        SizedString_SplitString(&afterOpenParen, ')', &beforeCloseParen, &afterCloseParen);
        __try
            addressingMode.expression = ExpressionEvalSizedString(pAssembler, &beforeCloseParen);
        __catch
            __rethrow_and_return(addressingMode);
            
        addressingMode.mode = ADDRESSING_MODE_INDIRECT;
        return addressingMode;
    }
    else if (pComma && !pOpenParen && !pCloseParen)
    {
        SizedString beforeComma;
        SizedString afterComma;
        
        SizedString_SplitString(&operandsString, ',', &beforeComma, &afterComma);
        if (0 == SizedString_strcasecmp(&afterComma, "X"))
        {
            __try
                addressingMode.expression = ExpressionEvalSizedString(pAssembler, &beforeComma);
            __catch
                __rethrow_and_return(addressingMode);
                
            addressingMode.mode = ADDRESSING_MODE_ABSOLUTE_INDEXED_X;
            return addressingMode;
        }
        else if (0 == SizedString_strcasecmp(&afterComma, "Y"))
        {
            __try
                addressingMode.expression = ExpressionEvalSizedString(pAssembler, &beforeComma);
            __catch
                __rethrow_and_return(addressingMode);

            addressingMode.mode = ADDRESSING_MODE_ABSOLUTE_INDEXED_Y;
            return addressingMode;
        }
    }
    else if (usesImmediateAddressing(pOperands))
    {
        __try
            addressingMode.expression = ExpressionEval(pAssembler, pOperands);
        __catch
            __rethrow_and_return(addressingMode);
            
        addressingMode.mode = ADDRESSING_MODE_IMMEDIATE;
        return addressingMode;
    }
    else
    {
        __try
            addressingMode.expression = ExpressionEval(pAssembler, pOperands);
        __catch
            __rethrow_and_return(addressingMode);
            
        addressingMode.mode = ADDRESSING_MODE_ABSOLUTE;
        return addressingMode;
    }
    
    addressingMode.mode = ADDRESSING_MODE_INVALID;
    __throw_and_return(invalidArgumentException, addressingMode);
}

static int usesImpliedAddressing(const char* pOperands)
{
    return pOperands == NULL;
}

static int usesImmediateAddressing(const char* pOperands)
{
    return pOperands[0] == '#';
}
