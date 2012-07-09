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

// Include headers from C modules under test.
extern "C"
{
    #include "ExpressionEval.h"
    #include "printfSpy.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(ExpressionEval)
{
    Expression   m_expression;
    Assembler*   m_pAssembler;
    char         m_buffer[64];
    
    void setup()
    {
        printfSpy_Hook(128);
        m_pAssembler = Assembler_CreateFromString(dupe("* First line of sample.\n"));
    }

    void teardown()
    {
        printfSpy_Unhook();
        Assembler_Free(m_pAssembler);
        LONGS_EQUAL(noException, getExceptionCode());
    }
    
    char* dupe(const char* pString)
    {
        strcpy(m_buffer, pString);
        return m_buffer;
    }
    
    void validateFailureMessage(const char* pExpectedErrorMessage, int expectedExceptionCode)
    {
        STRCMP_EQUAL(pExpectedErrorMessage, printfSpy_GetLastOutput());
        POINTERS_EQUAL(stderr, printfSpy_GetLastFile());
        LONGS_EQUAL(1, printfSpy_GetCallCount());
        LONGS_EQUAL(expectedExceptionCode, getExceptionCode());
        clearExceptionCode();
    }
};


TEST(ExpressionEval, EvaluateHexLiteral)
{
    m_expression = ExpressionEval(m_pAssembler, "$AF09");
    LONGS_EQUAL(0xaf09, m_expression.value);
    LONGS_EQUAL(TYPE_ABSOLUTE, m_expression.type);
}

TEST(ExpressionEval, EvaluateInvalidHexDigit)
{
    m_expression = ExpressionEval(m_pAssembler, "$AG");
    LONGS_EQUAL(0xA, m_expression.value);
    LONGS_EQUAL(TYPE_ZEROPAGE_ABSOLUTE, m_expression.type);
}

TEST(ExpressionEval, EvaluateHexValueTooLong)
{
    m_expression = ExpressionEval(m_pAssembler, "$12345");
    LONGS_EQUAL(0x0, m_expression.value);
    validateFailureMessage("filename:0: error: Hexadecimal number '$12345' doesn't fit in 16-bits.\n", invalidArgumentException);
}

TEST(ExpressionEval, EvaluateHexImmediate)
{
    m_expression = ExpressionEval(m_pAssembler, "#$60");
    LONGS_EQUAL(0x60, m_expression.value);
    LONGS_EQUAL(TYPE_IMMEDIATE, m_expression.type);
}

TEST(ExpressionEval, EvaluateBinaryLiteral)
{
    m_expression = ExpressionEval(m_pAssembler, "%1010111100001001");
    LONGS_EQUAL(0xaf09, m_expression.value);
    LONGS_EQUAL(TYPE_ABSOLUTE, m_expression.type);
}

TEST(ExpressionEval, EvaluateInvalidBinaryDigit)
{
    m_expression = ExpressionEval(m_pAssembler, "%12");
    LONGS_EQUAL(0x1, m_expression.value);
    LONGS_EQUAL(TYPE_ZEROPAGE_ABSOLUTE, m_expression.type);
}

TEST(ExpressionEval, EvaluateBinaryValueTooLong)
{
    m_expression = ExpressionEval(m_pAssembler, "%11110000111100001");
    LONGS_EQUAL(0x0, m_expression.value);
    validateFailureMessage("filename:0: error: Binary number '%11110000111100001' doesn't fit in 16-bits.\n", invalidArgumentException);
}

TEST(ExpressionEval, EvaluateBinaryImmediate)
{
    m_expression = ExpressionEval(m_pAssembler, "#%11110000");
    LONGS_EQUAL(0xF0, m_expression.value);
    LONGS_EQUAL(TYPE_IMMEDIATE, m_expression.type);
}

TEST(ExpressionEval, EvaluateDecimalLiteral)
{
    m_expression = ExpressionEval(m_pAssembler, "65535");
    LONGS_EQUAL(0xffff, m_expression.value);
    LONGS_EQUAL(TYPE_ABSOLUTE, m_expression.type);
}

TEST(ExpressionEval, EvaluateInvalidDecimalDigit)
{
    m_expression = ExpressionEval(m_pAssembler, "1f");
    LONGS_EQUAL(0x1, m_expression.value);
    LONGS_EQUAL(TYPE_ZEROPAGE_ABSOLUTE, m_expression.type);
}

TEST(ExpressionEval, EvaluateDecimalValueTooLong)
{
    m_expression = ExpressionEval(m_pAssembler, "65536");
    LONGS_EQUAL(0x0, m_expression.value);
    validateFailureMessage("filename:0: error: Decimal number '65536' doesn't fit in 16-bits.\n", invalidArgumentException);
}

TEST(ExpressionEval, EvaluateDecimalImmediate)
{
    m_expression = ExpressionEval(m_pAssembler, "#255");
    LONGS_EQUAL(0xFF, m_expression.value);
    LONGS_EQUAL(TYPE_IMMEDIATE, m_expression.type);
}

TEST(ExpressionEval, AsteriskinExpression)
{
    m_expression = ExpressionEval(m_pAssembler, dupe("*-CHECKER"));
    validateFailureMessage("filename:0: error: Unexpected prefix in '*-CHECKER' expression.\n", invalidArgumentException);
}

TEST(ExpressionEval, InvalidImmediateValueLargerThan8Bits)
{
    m_expression = ExpressionEval(m_pAssembler, dupe("#$100"));
    validateFailureMessage("filename:0: error: Immediate expression '$100' doesn't fit in 8-bits.\n", invalidArgumentException);
}

TEST(ExpressionEval, EvaluateSingleQuotedASCII)
{
    m_expression = ExpressionEval(m_pAssembler, "#'a'");
    LONGS_EQUAL('a', m_expression.value);
    LONGS_EQUAL(TYPE_IMMEDIATE, m_expression.type);
}

TEST(ExpressionEval, EvaluateSingleQuotedPrefixOnlyASCII)
{
    m_expression = ExpressionEval(m_pAssembler, "#'a");
    LONGS_EQUAL('a', m_expression.value);
    LONGS_EQUAL(TYPE_IMMEDIATE, m_expression.type);
}

TEST(ExpressionEval, EvaluateDoubleQuotedASCII)
{
    m_expression = ExpressionEval(m_pAssembler, "#\"a\"");
    LONGS_EQUAL('a' | 0x80, m_expression.value);
    LONGS_EQUAL(TYPE_IMMEDIATE, m_expression.type);
}

TEST(ExpressionEval, EvaluateDoubleQuotedPrefixOnlyASCII)
{
    m_expression = ExpressionEval(m_pAssembler, "#\"a");
    LONGS_EQUAL('a' | 0x80, m_expression.value);
    LONGS_EQUAL(TYPE_IMMEDIATE, m_expression.type);
}
