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
    const char* pEnd;
    const char* pNext;
} ExpressionEvaluation;

typedef void (*operatorHandler)(Assembler* pAssembler, ExpressionEvaluation* pEvalLeft, ExpressionEvaluation* pEvalRight);


static int isImmediatePrefix(char prefixChar);
static void parseImmediate(Assembler* pAssembler, ExpressionEvaluation* pEval);
static int isLowBytePrefix(char prefixChar);
static int isHighBytePrefix(char prefixChar);
static void expressionEval(Assembler* pAssembler, ExpressionEvaluation* pEval);
static void evaluatePrimitive(Assembler* pAssembler, ExpressionEvaluation* pEval);
static size_t stringLength(ExpressionEvaluation* pEval);
static void evaluateOperation(Assembler* pAssembler, ExpressionEvaluation* pEval);
static operatorHandler determineHandlerForOperator(Assembler* pAssembler, char operatorChar);
static void addHandler(Assembler* pAssembler, ExpressionEvaluation* pEvalLeft, ExpressionEvaluation* pEvalRight);
static void subtractHandler(Assembler* pAssembler, ExpressionEvaluation* pEvalLeft, ExpressionEvaluation* pEvalRight);
static void multiplyHandler(Assembler* pAssembler, ExpressionEvaluation* pEvalLeft, ExpressionEvaluation* pEvalRight);
static void divisionHandler(Assembler* pAssembler, ExpressionEvaluation* pEvalLeft, ExpressionEvaluation* pEvalRight);
static void xorHandler(Assembler* pAssembler, ExpressionEvaluation* pEvalLeft, ExpressionEvaluation* pEvalRight);
static void orHandler(Assembler* pAssembler, ExpressionEvaluation* pEvalLeft, ExpressionEvaluation* pEvalRight);
static void andHandler(Assembler* pAssembler, ExpressionEvaluation* pEvalLeft, ExpressionEvaluation* pEvalRight);
static void combineExpressionTypeAndFlags(Expression* pLeftExpression, Expression* pRightExpression);
static int isHexPrefix(char prefixChar);
static void parseHexValue(Assembler* pAssembler, ExpressionEvaluation* pEval);
static unsigned short parseHexDigit(char digit);
static int isBinaryPrefix(char prefixChar);
static void parseBinaryValue(Assembler* pAssembler, ExpressionEvaluation* pEval);
static unsigned short parseBinaryDigit(char digit);
static int isDecimal(char firstChar);
static void parseDecimalValue(Assembler* pAssembler, ExpressionEvaluation* pEval);
static unsigned short parseDecimalDigit(char digit);
static int isSingleQuoteASCII(char prefixChar);
static void parseASCIIValue(Assembler* pAssembler, ExpressionEvaluation* pEval);
static int isDoubleQuotedASCII(char prefixChar);
static int isCurrentAddressChar(char prefixChar);
static void parseCurrentAddressChar(Assembler* pAssembler, ExpressionEvaluation* pEval);
static int isUnarySubtractionOperator(char prefixChar);
static int isLabelReference(char prefixChar);
static void parseLabelReference(Assembler* pAssembler, ExpressionEvaluation* pEval);
static size_t lengthOfLabel(const char* pLabel);
static void flagForwardReferenceOnExpressionIfUndefinedLabel(ExpressionEvaluation* pEval, Symbol* pSymbol);
__throws Expression ExpressionEval(Assembler* pAssembler, const SizedString* pOperands)
{
    ExpressionEvaluation eval;
    
    memset(&eval.expression, 0, sizeof(eval.expression));
    eval.pCurrent = pOperands->pString;
    eval.pEnd = pOperands->pString + pOperands->stringLength;

    if (isImmediatePrefix(*eval.pCurrent))
        parseImmediate(pAssembler, &eval);
    else
        expressionEval(pAssembler, &eval);
    
    return eval.expression;
}

static int isImmediatePrefix(char prefixChar)
{
    return prefixChar == '#';
}

static void parseImmediate(Assembler* pAssembler, ExpressionEvaluation* pEval)
{
    char prefixChar;

    pEval->pCurrent++;
    prefixChar = *pEval->pCurrent;
    
    if (isLowBytePrefix(prefixChar))
    {
        pEval->pCurrent++;
        expressionEval(pAssembler, pEval);
        pEval->expression.value &= 0xff;
    }
    else if (isHighBytePrefix(prefixChar))
    {
        pEval->pCurrent++;
        expressionEval(pAssembler, pEval);
        pEval->expression.value >>= 8;
    }
    else
    {
        expressionEval(pAssembler, pEval);
    }
    pEval->expression.type = TYPE_IMMEDIATE;
}

static int isLowBytePrefix(char prefixChar)
{
    return prefixChar == '<';
}

static int isHighBytePrefix(char prefixChar)
{
    return prefixChar == '>' || prefixChar == '^';
}

static void expressionEval(Assembler* pAssembler, ExpressionEvaluation* pEval)
{

    __try
        evaluatePrimitive(pAssembler, pEval);
    __catch
        __rethrow;
        
    pEval->pCurrent = pEval->pNext;
    while (pEval->pCurrent < pEval->pEnd)
    {
        __try
            evaluateOperation(pAssembler, pEval);
        __catch
            __rethrow;
    }
}

static void evaluatePrimitive(Assembler* pAssembler, ExpressionEvaluation* pEval)
{
    char     prefixChar = *pEval->pCurrent;

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
    else if (isCurrentAddressChar(prefixChar))
    {
        parseCurrentAddressChar(pAssembler, pEval);
    }
    else if (isUnarySubtractionOperator(prefixChar))
    {
        pEval->pCurrent++;
        evaluatePrimitive(pAssembler, pEval);
        pEval->expression = ExpressionEval_CreateAbsoluteExpression(-pEval->expression.value);
    }
    else if (isLabelReference(prefixChar))
    {
        parseLabelReference(pAssembler, pEval);
    }
    else
    {
        LOG_ERROR(pAssembler, "Unexpected prefix in '%.*s' expression.", stringLength(pEval), pEval->pCurrent);
        __throw(invalidArgumentException);
    }
    
    pEval->pCurrent = pEval->pNext;
}

static size_t stringLength(ExpressionEvaluation* pEval)
{
    return pEval->pEnd - pEval->pCurrent;
}

static void evaluateOperation(Assembler* pAssembler, ExpressionEvaluation* pEval)
{
    __try
    {
        operatorHandler      handleOperator;
        ExpressionEvaluation rightEval;
        
        handleOperator = determineHandlerForOperator(pAssembler, *pEval->pCurrent);
        memset(&rightEval, 0, sizeof(rightEval));
        rightEval.pCurrent = pEval->pCurrent + 1;
        evaluatePrimitive(pAssembler, &rightEval);
        handleOperator(pAssembler, pEval, &rightEval);
        combineExpressionTypeAndFlags(&pEval->expression, &rightEval.expression);
        pEval->pCurrent = rightEval.pNext;
    }
    __catch
    {
        __rethrow;
    }
}

static operatorHandler determineHandlerForOperator(Assembler* pAssembler, char operatorChar)
{
    switch (operatorChar)
    {
    case '+':
        return addHandler;
    case '-':
        return subtractHandler;
    case '*':
        return multiplyHandler;
    case '/':
        return divisionHandler;
    case '!':
        return xorHandler;
    case '.':
        return orHandler;
    case '&':
        return andHandler;
    default:
        LOG_ERROR(pAssembler, "'%c' is unexpected operator.", operatorChar);
        __throw(invalidArgumentException);
    }
}

static void addHandler(Assembler* pAssembler, ExpressionEvaluation* pEvalLeft, ExpressionEvaluation* pEvalRight)
{
    pEvalLeft->expression.value += pEvalRight->expression.value;
}

static void subtractHandler(Assembler* pAssembler, ExpressionEvaluation* pEvalLeft, ExpressionEvaluation* pEvalRight)
{
    pEvalLeft->expression.value -= pEvalRight->expression.value;
}

static void multiplyHandler(Assembler* pAssembler, ExpressionEvaluation* pEvalLeft, ExpressionEvaluation* pEvalRight)
{
    pEvalLeft->expression.value *= pEvalRight->expression.value;
}

static void divisionHandler(Assembler* pAssembler, ExpressionEvaluation* pEvalLeft, ExpressionEvaluation* pEvalRight)
{
    pEvalLeft->expression.value /= pEvalRight->expression.value;
}

static void xorHandler(Assembler* pAssembler, ExpressionEvaluation* pEvalLeft, ExpressionEvaluation* pEvalRight)
{
    pEvalLeft->expression.value ^= pEvalRight->expression.value;
}

static void orHandler(Assembler* pAssembler, ExpressionEvaluation* pEvalLeft, ExpressionEvaluation* pEvalRight)
{
    pEvalLeft->expression.value |= pEvalRight->expression.value;
}

static void andHandler(Assembler* pAssembler, ExpressionEvaluation* pEvalLeft, ExpressionEvaluation* pEvalRight)
{
    pEvalLeft->expression.value &= pEvalRight->expression.value;
}

static void combineExpressionTypeAndFlags(Expression* pLeftExpression, Expression* pRightExpression)
{
    pLeftExpression->flags |= pRightExpression->flags;
    if (pRightExpression->type < pLeftExpression->type)
        pLeftExpression->type = pRightExpression->type;
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
        __throw(invalidHexDigitException);
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
        __throw(invalidBinaryDigitException);
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
        __throw(invalidDecimalDigitException);
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

static int isCurrentAddressChar(char prefixChar)
{
    return prefixChar == '*';
}

static void parseCurrentAddressChar(Assembler* pAssembler, ExpressionEvaluation* pEval)
{
    pEval->pNext = pEval->pCurrent + 1;
    pEval->expression = ExpressionEval_CreateAbsoluteExpression(pAssembler->programCounter);
}

static int isUnarySubtractionOperator(char prefixChar)
{
    return prefixChar == '-';
}

static int isLabelReference(char prefixChar)
{
    return prefixChar >= ':';
}

static void parseLabelReference(Assembler* pAssembler, ExpressionEvaluation* pEval)
{
    Symbol* pSymbol = NULL;
    size_t  labelLength = lengthOfLabel(pEval->pCurrent);

    __try
        pSymbol = Assembler_FindLabel(pAssembler, pEval->pCurrent, labelLength);
    __catch
        __rethrow;

    pEval->expression = pSymbol->expression;
    pEval->pNext = pEval->pCurrent + labelLength;
    flagForwardReferenceOnExpressionIfUndefinedLabel(pEval, pSymbol);
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

static void flagForwardReferenceOnExpressionIfUndefinedLabel(ExpressionEvaluation* pEval, Symbol* pSymbol)
{
    if (pSymbol->pDefinedLine == NULL)
        pEval->expression.flags |= EXPRESSION_FLAG_FORWARD_REFERENCE;
}


Expression ExpressionEval_CreateAbsoluteExpression(unsigned short value)
{
    Expression expression;
    
    memset(&expression, 0, sizeof(expression));
    expression.value = value;
    if (value <= 0xff)
        expression.type = TYPE_ZEROPAGE;
    else
        expression.type = TYPE_ABSOLUTE;
    return expression;
}
