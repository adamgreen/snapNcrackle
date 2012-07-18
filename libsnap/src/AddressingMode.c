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


typedef struct CharLocations
{
    const char* pStart;
    const char* pEnd;
    const char* pComma;
    const char* pOpenParen;
    const char* pCloseParen;
}
CharLocations;


static CharLocations initCharLocations(SizedString* pOperandsString);
static AddressingMode initializedAddressingModeStruct(AddressingModes mode);
static int usesImpliedAddressing(const char* pOperands);
static AddressingMode impliedAddressing(void);
static int usesIndexedIndirectAddressing(CharLocations* pLocations);
static int hasParensAndComma(CharLocations* pLocations);
static int hasOpeningParenAtBeginning(CharLocations* pLocations);
static int hasClosingParenAtEnding(CharLocations* pLocations);
static int isCommaAfterOpeningParen(CharLocations* pLocations);
static int isClosingParenAfterComma(CharLocations* pLocations);
static AddressingMode indexedIndirectAddressing(Assembler* pAssembler, SizedString* pOperandsString);
static int usesIndirectIndexedAddressing(CharLocations* pLocations);
static int isCommaAfterClosingParen(CharLocations* pLocations);
static AddressingMode indirectIndexedAddressing(Assembler* pAssembler, SizedString* pOperandsString);
static int usesIndirectAddressing(CharLocations* pLocations);
static int hasParensAndNoComma(CharLocations* pLocations);
static AddressingMode indirectAddressing(Assembler* pAssembler, SizedString* pOperandsString);
static int usesIndexedAddressing(CharLocations* pLocations);
static int hasCommaAndNoParens(CharLocations* pLocations);
static AddressingMode indexedAddressing(Assembler* pAssembler, SizedString* pOperandsString);
static int hasNoCommaOrParens(CharLocations* pLocations);
static AddressingMode immediateOrAbsoluteAddressing(Assembler* pAssembler, SizedString* pOperandsString);
static int usesImmediateAddressing(SizedString* pOperandsString);
__throws AddressingMode AddressingMode_Eval(Assembler* pAssembler, const char* pOperands)
{
    SizedString    operandsString = SizedString_InitFromString(pOperands);
    CharLocations  locations = initCharLocations(&operandsString);
    AddressingMode addressingMode = initializedAddressingModeStruct(ADDRESSING_MODE_INVALID);

    if (usesImpliedAddressing(pOperands))
        return impliedAddressing();
    else if (usesIndexedIndirectAddressing(&locations))
        return indexedIndirectAddressing(pAssembler, &operandsString);
    else if (usesIndirectIndexedAddressing(&locations))
        return indirectIndexedAddressing(pAssembler, &operandsString);
    else if (usesIndirectAddressing(&locations))
        return indirectAddressing(pAssembler, &operandsString);
    else if (usesIndexedAddressing(&locations))
        return indexedAddressing(pAssembler, &operandsString);
    else if (hasNoCommaOrParens(&locations))
        return immediateOrAbsoluteAddressing(pAssembler, &operandsString);
    else
        __throw_and_return(invalidArgumentException, addressingMode);
}

static CharLocations initCharLocations(SizedString* pOperandsString)
{
    CharLocations locations;

    locations.pStart = pOperandsString->pString;
    locations.pEnd = pOperandsString->pString + pOperandsString->stringLength - 1;
    locations.pComma = SizedString_strchr(pOperandsString, ',');
    locations.pOpenParen = SizedString_strchr(pOperandsString, '(');
    locations.pCloseParen = SizedString_strchr(pOperandsString, ')');
    
    return locations;
}

static AddressingMode initializedAddressingModeStruct(AddressingModes mode)
{
    AddressingMode addressingMode;
    
    memset(&addressingMode, 0, sizeof(addressingMode));
    addressingMode.mode = mode;
    return addressingMode;
}

static int usesImpliedAddressing(const char* pOperands)
{
    return pOperands == NULL;
}

static AddressingMode impliedAddressing(void)
{
    return initializedAddressingModeStruct(ADDRESSING_MODE_IMPLIED);
}

static int usesIndexedIndirectAddressing(CharLocations* pLocations)
{
    return hasParensAndComma(pLocations) && 
           hasOpeningParenAtBeginning(pLocations) && hasClosingParenAtEnding(pLocations) &&
           isCommaAfterOpeningParen(pLocations) && isClosingParenAfterComma(pLocations);
}

static int hasParensAndComma(CharLocations* pLocations)
{
    return pLocations->pOpenParen && pLocations->pCloseParen && pLocations->pComma;
}

static int hasOpeningParenAtBeginning(CharLocations* pLocations)
{
    return pLocations->pOpenParen == pLocations->pStart;
}

static int hasClosingParenAtEnding(CharLocations* pLocations)
{
    return pLocations->pCloseParen == pLocations->pEnd;
}

static int isCommaAfterOpeningParen(CharLocations* pLocations)
{
    return pLocations->pComma > pLocations->pOpenParen;
}

static int isClosingParenAfterComma(CharLocations* pLocations)
{
    return pLocations->pCloseParen > pLocations->pComma;
}

static AddressingMode indexedIndirectAddressing(Assembler* pAssembler, SizedString* pOperandsString)
{
    AddressingMode addressingMode = initializedAddressingModeStruct(ADDRESSING_MODE_INVALID);
    SizedString    beforeOpenParen;
    SizedString    afterOpenParen;
    SizedString    beforeComma;
    SizedString    afterComma;

    SizedString_SplitString(pOperandsString, '(', &beforeOpenParen, &afterOpenParen);
    SizedString_SplitString(&afterOpenParen, ',', &beforeComma, &afterComma);
    if (0 != SizedString_strcasecmp(&afterComma, "X)"))
        __throw_and_return(invalidArgumentException, addressingMode);

    __try
        addressingMode.expression = ExpressionEvalSizedString(pAssembler, &beforeComma);
    __catch
        __rethrow_and_return(addressingMode);

    addressingMode.mode = ADDRESSING_MODE_INDEXED_INDIRECT;
    return addressingMode;
}

static int usesIndirectIndexedAddressing(CharLocations* pLocations)
{
    return hasParensAndComma(pLocations) && hasOpeningParenAtBeginning(pLocations) && 
           isCommaAfterOpeningParen(pLocations) && isCommaAfterClosingParen(pLocations);
}

static int isCommaAfterClosingParen(CharLocations* pLocations)
{
    return !isClosingParenAfterComma(pLocations);
}

static AddressingMode indirectIndexedAddressing(Assembler* pAssembler, SizedString* pOperandsString)
{
    AddressingMode addressingMode = initializedAddressingModeStruct(ADDRESSING_MODE_INVALID);
    SizedString    beforeOpenParen;
    SizedString    afterOpenParen;
    SizedString    beforeCloseParen;
    SizedString    afterCloseParen;

    SizedString_SplitString(pOperandsString, '(', &beforeOpenParen, &afterOpenParen);
    SizedString_SplitString(&afterOpenParen, ')', &beforeCloseParen, &afterCloseParen);
    if (0 != SizedString_strcasecmp(&afterCloseParen, ",Y"))
        __throw_and_return(invalidArgumentException, addressingMode);
        
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

static int usesIndirectAddressing(CharLocations* pLocations)
{
    return hasParensAndNoComma(pLocations) && 
           hasOpeningParenAtBeginning(pLocations) && hasClosingParenAtEnding(pLocations);
}

static int hasParensAndNoComma(CharLocations* pLocations)
{
    return pLocations->pOpenParen && pLocations->pCloseParen && !pLocations->pComma;
}

static AddressingMode indirectAddressing(Assembler* pAssembler, SizedString* pOperandsString)
{
    AddressingMode addressingMode = initializedAddressingModeStruct(ADDRESSING_MODE_INVALID);
    SizedString beforeOpenParen;
    SizedString afterOpenParen;
    SizedString beforeCloseParen;
    SizedString afterCloseParen;

    SizedString_SplitString(pOperandsString, '(', &beforeOpenParen, &afterOpenParen);
    SizedString_SplitString(&afterOpenParen, ')', &beforeCloseParen, &afterCloseParen);
    __try
        addressingMode.expression = ExpressionEvalSizedString(pAssembler, &beforeCloseParen);
    __catch
        __rethrow_and_return(addressingMode);
        
    addressingMode.mode = ADDRESSING_MODE_INDIRECT;
    return addressingMode;
}

static int usesIndexedAddressing(CharLocations* pLocations)
{
    return hasCommaAndNoParens(pLocations);
}

static int hasCommaAndNoParens(CharLocations* pLocations)
{
    return pLocations->pComma && !pLocations->pOpenParen && !pLocations->pCloseParen;
}

static AddressingMode indexedAddressing(Assembler* pAssembler, SizedString* pOperandsString)
{
    AddressingMode addressingMode = initializedAddressingModeStruct(ADDRESSING_MODE_INVALID);
    SizedString    beforeComma;
    SizedString    afterComma;
    
    SizedString_SplitString(pOperandsString, ',', &beforeComma, &afterComma);
    if (0 == SizedString_strcasecmp(&afterComma, "X"))
        addressingMode.mode = ADDRESSING_MODE_ABSOLUTE_INDEXED_X;
    else if (0 == SizedString_strcasecmp(&afterComma, "Y"))
        addressingMode.mode = ADDRESSING_MODE_ABSOLUTE_INDEXED_Y;
    else
        __throw_and_return(invalidArgumentException, addressingMode);
    
    __try
    {
        addressingMode.expression = ExpressionEvalSizedString(pAssembler, &beforeComma);
    }
    __catch
    {
        addressingMode.mode = ADDRESSING_MODE_INVALID;
        __rethrow_and_return(addressingMode);
    }
            
    return addressingMode;
}

static int hasNoCommaOrParens(CharLocations* pLocations)
{
    return !pLocations->pComma && !pLocations->pOpenParen && !pLocations->pCloseParen;
}

static AddressingMode immediateOrAbsoluteAddressing(Assembler* pAssembler, SizedString* pOperandsString)
{
    AddressingMode addressingMode = initializedAddressingModeStruct(ADDRESSING_MODE_INVALID);
    
    if (usesImmediateAddressing(pOperandsString))
        addressingMode.mode = ADDRESSING_MODE_IMMEDIATE;
    else
        addressingMode.mode = ADDRESSING_MODE_ABSOLUTE;

    __try
    {
        addressingMode.expression = ExpressionEvalSizedString(pAssembler, pOperandsString);
    }
    __catch
    {
        addressingMode.mode = ADDRESSING_MODE_INVALID;
        __rethrow_and_return(addressingMode);
    }

    return addressingMode;
}

static int usesImmediateAddressing(SizedString* pOperandsString)
{
    return pOperandsString->pString[0] == '#';
}
