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
#include <ctype.h>
#include <string.h>
#include "AddressingMode.h"
#include "AddressingModeTest.h"
#include "SizedString.h"
#include "AssemblerPriv.h"


typedef struct CharLocations
{
    const char* pStart;
    const char* pComma;
    const char* pOpenParen;
    const char* pCloseParen;
}
CharLocations;


static CharLocations initCharLocations(SizedString* pOperandsString);
static AddressingMode initializedAddressingModeStruct(AddressingModes mode);
static int usesImpliedAddressing(SizedString* pOperands);
static AddressingMode impliedAddressing(void);
static int usesIndexedIndirectAddressing(CharLocations* pLocations);
static int hasParensAndComma(CharLocations* pLocations);
static int hasOpeningParenAtBeginning(CharLocations* pLocations);
static int isCommaAfterOpeningParen(CharLocations* pLocations);
static int isClosingParenAfterComma(CharLocations* pLocations);
static AddressingMode indexedIndirectAddressing(Assembler* pAssembler, SizedString* pOperandsString);
static int usesIndirectIndexedAddressing(CharLocations* pLocations);
static int isCommaAfterClosingParen(CharLocations* pLocations);
static void truncateAtFirstWhitespace(SizedString* pString);
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


#define reportAndThrowOnInvalidAddressingMode(pAssembler, pOperands) \
{ \
    LOG_ERROR(pAssembler, "'%.*s' doesn't represent a known addressing mode.", \
              (pOperands)->stringLength, (pOperands)->pString); \
    __throw(invalidArgumentException); \
}

#define reportAndThrowOnInvalidIndexRegister(pAssembler, pIndexRegister) \
{ \
    LOG_ERROR(pAssembler, "'%.*s' isn't a valid index register for this addressing mode.", \
              (pIndexRegister)->stringLength, (pIndexRegister)->pString); \
    __throw(invalidArgumentException); \
}


__throws AddressingMode AddressingMode_Eval(Assembler* pAssembler, SizedString* pOperands)
{
    CharLocations  locations = initCharLocations(pOperands);

    if (usesImpliedAddressing(pOperands))
        return impliedAddressing();
    else if (usesIndexedIndirectAddressing(&locations))
        return indexedIndirectAddressing(pAssembler, pOperands);
    else if (usesIndirectIndexedAddressing(&locations))
        return indirectIndexedAddressing(pAssembler, pOperands);
    else if (usesIndirectAddressing(&locations))
        return indirectAddressing(pAssembler, pOperands);
    else if (usesIndexedAddressing(&locations))
        return indexedAddressing(pAssembler, pOperands);
    else if (hasNoCommaOrParens(&locations))
        return immediateOrAbsoluteAddressing(pAssembler, pOperands);
    else
        reportAndThrowOnInvalidAddressingMode(pAssembler, pOperands);
}

static CharLocations initCharLocations(SizedString* pOperandsString)
{
    CharLocations locations;

    locations.pStart = pOperandsString->pString;
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

static int usesImpliedAddressing(SizedString* pOperands)
{
    return SizedString_strlen(pOperands) == 0;
}

static AddressingMode impliedAddressing(void)
{
    return initializedAddressingModeStruct(ADDRESSING_MODE_IMPLIED);
}

static int usesIndexedIndirectAddressing(CharLocations* pLocations)
{
    return hasParensAndComma(pLocations) && 
           hasOpeningParenAtBeginning(pLocations) &&
           isCommaAfterOpeningParen(pLocations) && 
           isClosingParenAfterComma(pLocations);
}

static int hasParensAndComma(CharLocations* pLocations)
{
    return pLocations->pOpenParen && pLocations->pCloseParen && pLocations->pComma;
}

static int hasOpeningParenAtBeginning(CharLocations* pLocations)
{
    return pLocations->pOpenParen == pLocations->pStart;
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
    SizedString    indexRegister;
    SizedString    afterClosingParen;

    SizedString_SplitString(pOperandsString, '(', &beforeOpenParen, &afterOpenParen);
    SizedString_SplitString(&afterOpenParen, ',', &beforeComma, &afterComma);
    SizedString_SplitString(&afterComma, ')', &indexRegister, &afterClosingParen);
    if (0 != SizedString_strcasecmp(&indexRegister, "X"))
        reportAndThrowOnInvalidIndexRegister(pAssembler, &indexRegister);

    addressingMode.expression = ExpressionEval(pAssembler, &beforeComma);
    addressingMode.mode = ADDRESSING_MODE_INDEXED_INDIRECT;
    return addressingMode;
}

static int usesIndirectIndexedAddressing(CharLocations* pLocations)
{
    return hasParensAndComma(pLocations) && hasOpeningParenAtBeginning(pLocations) && 
           isCommaAfterClosingParen(pLocations);
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
    SizedString    beforeComma;
    SizedString    indexRegister;

    SizedString_SplitString(pOperandsString, '(', &beforeOpenParen, &afterOpenParen);
    SizedString_SplitString(&afterOpenParen, ')', &beforeCloseParen, &afterCloseParen);
    SizedString_SplitString(&afterCloseParen, ',', &beforeComma, &indexRegister);
    truncateAtFirstWhitespace(&indexRegister);
    if (0 != SizedString_strcasecmp(&indexRegister, "Y"))
        reportAndThrowOnInvalidIndexRegister(pAssembler, &indexRegister);
        
    addressingMode.expression = ExpressionEval(pAssembler, &beforeCloseParen);
    if (addressingMode.expression.type != TYPE_ZEROPAGE)
    {
        LOG_ERROR(pAssembler, "'%.*s' isn't in page zero as required for indirect indexed addressing.", 
                  beforeCloseParen.stringLength, beforeCloseParen.pString);
        __throw(invalidArgumentException);
    }
    addressingMode.mode = ADDRESSING_MODE_INDIRECT_INDEXED;
    return addressingMode;
}

static void truncateAtFirstWhitespace(SizedString* pString)
{
    const char* pCurr;

    SizedString_EnumStart(pString, &pCurr);
    while (SizedString_EnumRemaining(pString, pCurr) > 0 && !isspace(SizedString_EnumCurr(pString, pCurr)))
        SizedString_EnumNext(pString, &pCurr);
    pString->stringLength = pCurr - pString->pString;
}

static int usesIndirectAddressing(CharLocations* pLocations)
{
    return hasParensAndNoComma(pLocations) && 
           hasOpeningParenAtBeginning(pLocations);
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

    addressingMode.expression = ExpressionEval(pAssembler, &beforeCloseParen);
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
    
    __try
    {
        SizedString_SplitString(pOperandsString, ',', &beforeComma, &afterComma);
        truncateAtFirstWhitespace(&afterComma);

        if (0 == SizedString_strcasecmp(&afterComma, "X"))
            addressingMode.mode = ADDRESSING_MODE_ABSOLUTE_INDEXED_X;
        else if (0 == SizedString_strcasecmp(&afterComma, "Y"))
            addressingMode.mode = ADDRESSING_MODE_ABSOLUTE_INDEXED_Y;
        else
            reportAndThrowOnInvalidIndexRegister(pAssembler, &afterComma);
        addressingMode.expression = ExpressionEval(pAssembler, &beforeComma);
    }
    __catch
    {
        addressingMode.mode = ADDRESSING_MODE_INVALID;
        __rethrow;
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
        addressingMode.expression = ExpressionEval(pAssembler, pOperandsString);
    }
    __catch
    {
        addressingMode.mode = ADDRESSING_MODE_INVALID;
        __rethrow;
    }

    return addressingMode;
}

static int usesImmediateAddressing(SizedString* pOperandsString)
{
    return pOperandsString->pString[0] == '#';
}
