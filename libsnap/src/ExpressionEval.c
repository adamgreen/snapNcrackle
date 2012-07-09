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
#include <limits.h>
#include "ExpressionEval.h"
#include "ExpressionEvalTest.h"
#include "AssemblerPriv.h"


typedef struct ExpressionEvaluation
{
    Expression  expression;
    const char* pCurrent;
    const char* pNext;
} ExpressionEvaluation;


static void expressionEval(Assembler* pAssembler, ExpressionEvaluation* pEval);
static int isHexPrefix(char prefixChar);
static void parseHexValue(Assembler* pAssembler, ExpressionEvaluation* pEval);
static unsigned short parseHexDigit(char digit);
static int isBinaryPrefix(char prefixChar);
static void parseBinaryValue(Assembler* pAssembler, ExpressionEvaluation* pEval);
static unsigned short parseBinaryDigit(char digit);
static int isDecimal(char firstChar);
static void parseDecimalValue(Assembler* pAssembler, ExpressionEvaluation* pEval);
static unsigned short parseDecimalDigit(char digit);
static int isImmediatePrefix(char prefixChar);
static int isLabelReference(char prefixChar);
static size_t lengthOfLabel(const char* pLabel);
static int isSingleQuoteASCII(char prefixChar);
static void parseASCIIValue(Assembler* pAssembler, ExpressionEvaluation* pEval);
static int isDoubleQuotedASCII(char prefixChar);
__throws Expression ExpressionEval(Assembler* pAssembler, const char* pOperands)
{
    ExpressionEvaluation eval;
    
    memset(&eval.expression, 0, sizeof(eval.expression));
    eval.pCurrent = pOperands;
    expressionEval(pAssembler, &eval);
    
    return eval.expression;
}

static void expressionEval(Assembler* pAssembler, ExpressionEvaluation* pEval)
{
    char     prefixChar = *pEval->pCurrent;
    Symbol*  pSymbol = NULL;
    
    if (isHexPrefix(prefixChar))
    {
        parseHexValue(pAssembler, pEval);
    }
    else if (isBinaryPrefix(prefixChar))
    {
        parseBinaryValue(pAssembler, pEval);
    }
    else if (isDecimal(prefixChar))
    {
        parseDecimalValue(pAssembler, pEval);
    }
    else if (isSingleQuoteASCII(prefixChar) || isDoubleQuotedASCII(prefixChar))
    {
        parseASCIIValue(pAssembler, pEval);
    }
    else if (isImmediatePrefix(prefixChar))
    {
        pEval->pCurrent++;
        expressionEval(pAssembler, pEval);
        pEval->expression.type = TYPE_IMMEDIATE;
        if (pEval->expression.value > 0xFF)
        {
            LOG_ERROR(pAssembler, "Immediate expression '%s' doesn't fit in 8-bits.", pEval->pCurrent);
            __throw(invalidArgumentException);
        }
    }
    else if (isLabelReference(prefixChar))
    {
        size_t labelLength = lengthOfLabel(pEval->pCurrent);
        pSymbol = Assembler_FindLabel(pAssembler, pEval->pCurrent, labelLength);
        if (!pSymbol)
            __throw(invalidArgumentException);
        pEval->expression = pSymbol->expression;
    }
    else
    {
        LOG_ERROR(pAssembler, "Unexpected prefix in '%s' expression.", pEval->pCurrent);
        __throw(invalidArgumentException);
    }
}

static int isHexPrefix(char prefixChar)
{
    return prefixChar == '$';
}

typedef struct Parser
{
    const char*    pType;
    unsigned short (*parseDigit)(char digit);
    unsigned short multiplier;
    int            skipPrefix;
} Parser;

static void parseValue(Assembler* pAssembler, ExpressionEvaluation* pEval, Parser* pParser)
{
    const char*    pCurrent = pEval->pCurrent + pParser->skipPrefix;
    unsigned int   value = 0;
    unsigned int   digitCount = 0;
    int            overflowDetected = FALSE;
    
    while (*pCurrent)
    {
        unsigned short digit;

        __try
        {
            digit = pParser->parseDigit(*pCurrent);
        }
        __catch
        {
            clearExceptionCode();
            break;
        }
        
        value = (value * pParser->multiplier) + digit;
        if (value > USHRT_MAX)
            overflowDetected = TRUE;
        digitCount++;
        pCurrent++;
    }
    
    if (overflowDetected)
    {
        LOG_ERROR(pAssembler, "%s number '%.*s' doesn't fit in 16-bits.", pParser->pType, digitCount+1, pEval->pCurrent);
        setExceptionCode(invalidArgumentException);
        value = 0;
    }
    pEval->pNext = pCurrent;
    pEval->expression = ExpressionEval_CreateAbsoluteExpression(value);
}

static void parseHexValue(Assembler* pAssembler, ExpressionEvaluation* pEval)
{
    static Parser hexParser =
    {
        "Hexadecimal",
        parseHexDigit,
        16,
        1
    };

    parseValue(pAssembler, pEval, &hexParser);
}

static unsigned short parseHexDigit(char digit)
{
    if (digit >= '0' && digit <= '9')
        return digit - '0';
    else if (digit >= 'a' && digit <= 'f')
        return digit - 'a' + 10;
    else if (digit >= 'A' && digit <= 'F')
        return digit - 'A' + 10;
    else
        __throw_and_return(invalidHexDigitException, 0);
}

static int isBinaryPrefix(char prefixChar)
{
    return prefixChar == '%';
}

static void parseBinaryValue(Assembler* pAssembler, ExpressionEvaluation* pEval)
{
    static Parser binaryParser =
    {
        "Binary",
        parseBinaryDigit,
        2,
        1
    };

    parseValue(pAssembler, pEval, &binaryParser);
}

static unsigned short parseBinaryDigit(char digit)
{
    if (digit >= '0' && digit <= '1')
        return digit - '0';
    else
        __throw_and_return(invalidBinaryDigitException, 0);
}

static int isDecimal(char firstChar)
{
    return firstChar >= '0' && firstChar <= '9';
}

static void parseDecimalValue(Assembler* pAssembler, ExpressionEvaluation* pEval)
{
    static Parser decimalParser =
    {
        "Decimal",
        parseDecimalDigit,
        10,
        0
    };

    parseValue(pAssembler, pEval, &decimalParser);
}

static unsigned short parseDecimalDigit(char digit)
{
    if (digit >= '0' && digit <= '9')
        return digit - '0';
    else
        __throw_and_return(invalidDecimalDigitException, 0);
}

static int isImmediatePrefix(char prefixChar)
{
    return prefixChar == '#';
}

static int isLabelReference(char prefixChar)
{
    return prefixChar >= ':';
}

static size_t lengthOfLabel(const char* pLabel)
{
    const char* pCurr = pLabel + 1;
    while (*pCurr && *pCurr >= '0')
    {
        pCurr++;
    }
    return pCurr - pLabel;
}

static int isSingleQuoteASCII(char prefixChar)
{
    return prefixChar == '\'';
}

static void parseASCIIValue(Assembler* pAssembler, ExpressionEvaluation* pEval)
{
    char          delimiter = pEval->pCurrent[0];
    int           forceHighBit = delimiter == '"';
    unsigned char value = forceHighBit ? pEval->pCurrent[1] | 0x80 : pEval->pCurrent[1];
    int           skipTrailingQuote = pEval->pCurrent[2] == delimiter;
    
    pEval->pNext = skipTrailingQuote ? &pEval->pCurrent[3] : &pEval->pCurrent[2];
    pEval->expression = ExpressionEval_CreateAbsoluteExpression(value);
}

static int isDoubleQuotedASCII(char prefixChar)
{
    return prefixChar == '"';
}


Expression ExpressionEval_CreateAbsoluteExpression(unsigned short value)
{
    Expression expression;
    
    expression.value = value;
    if (value <= 0xff)
        expression.type = TYPE_ZEROPAGE_ABSOLUTE;
    else
        expression.type = TYPE_ABSOLUTE;
    return expression;
}
