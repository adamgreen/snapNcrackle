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
    #include "SymbolTable.h"
    #include "../src/AssemblerPriv.h"
    #include "printfSpy.h"
    #include "MallocFailureInject.h"
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
        m_pAssembler = NULL;
        setupAssemblerModule("");
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
    
    void setupAssemblerModule(const char* pAsmSource)
    {
        Assembler_Free(m_pAssembler);
        m_pAssembler = Assembler_CreateFromString(dupe(pAsmSource));
        CHECK(m_pAssembler != NULL);
        Assembler_Run(m_pAssembler);
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
    validateFailureMessage("filename:0: error: 'G' is unexpected operator.\n", invalidArgumentException);
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
    validateFailureMessage("filename:0: error: '2' is unexpected operator.\n", invalidArgumentException);
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
    validateFailureMessage("filename:0: error: 'f' is unexpected operator.\n", invalidArgumentException);
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
    m_expression = ExpressionEval(m_pAssembler, "#'a+1");
    LONGS_EQUAL('a'+1, m_expression.value);
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
    m_expression = ExpressionEval(m_pAssembler, "#\"a+1");
    LONGS_EQUAL(('a' | 0x80)+1, m_expression.value);
    LONGS_EQUAL(TYPE_IMMEDIATE, m_expression.type);
}

TEST(ExpressionEval, AsteriskInExpression)
{
    setupAssemblerModule(" org $800\n");
    m_expression = ExpressionEval(m_pAssembler, dupe("*"));
    LONGS_EQUAL(0x800, m_expression.value);
    LONGS_EQUAL(TYPE_ABSOLUTE, m_expression.type);
}

TEST(ExpressionEval, EvaluateAddition)
{
    m_expression = ExpressionEval(m_pAssembler, "2+3");
    LONGS_EQUAL(5, m_expression.value);
    LONGS_EQUAL(TYPE_ZEROPAGE_ABSOLUTE, m_expression.type);
}

TEST(ExpressionEval, EvaluateSubtraction)
{
    m_expression = ExpressionEval(m_pAssembler, "3-2");
    LONGS_EQUAL(1, m_expression.value);
    LONGS_EQUAL(TYPE_ZEROPAGE_ABSOLUTE, m_expression.type);
}

TEST(ExpressionEval, EvaluateMultiplication)
{
    m_expression = ExpressionEval(m_pAssembler, "3*2");
    LONGS_EQUAL(6, m_expression.value);
    LONGS_EQUAL(TYPE_ZEROPAGE_ABSOLUTE, m_expression.type);
}

TEST(ExpressionEval, EvaluateDivision)
{
    m_expression = ExpressionEval(m_pAssembler, "3/2");
    LONGS_EQUAL(1, m_expression.value);
    LONGS_EQUAL(TYPE_ZEROPAGE_ABSOLUTE, m_expression.type);
}

TEST(ExpressionEval, EvaluateNoOperatorPrecedenceExample)
{
    m_expression = ExpressionEval(m_pAssembler, "2+3*5");
    LONGS_EQUAL(25, m_expression.value);
    LONGS_EQUAL(TYPE_ZEROPAGE_ABSOLUTE, m_expression.type);
}

TEST(ExpressionEval, EvaluateXOR)
{
    m_expression = ExpressionEval(m_pAssembler, "$ff!$80");
    LONGS_EQUAL(0x7f, m_expression.value);
    LONGS_EQUAL(TYPE_ZEROPAGE_ABSOLUTE, m_expression.type);
}

TEST(ExpressionEval, EvaluateOR)
{
    m_expression = ExpressionEval(m_pAssembler, "$00.$80");
    LONGS_EQUAL(0x80, m_expression.value);
    LONGS_EQUAL(TYPE_ZEROPAGE_ABSOLUTE, m_expression.type);
}

TEST(ExpressionEval, EvaluateAND)
{
    m_expression = ExpressionEval(m_pAssembler, "$af&$f0");
    LONGS_EQUAL(0xa0, m_expression.value);
    LONGS_EQUAL(TYPE_ZEROPAGE_ABSOLUTE, m_expression.type);
}

TEST(ExpressionEval, ForwardLabelReference)
{
    static const char labelName[] = "fwd_label";
    m_expression = ExpressionEval(m_pAssembler, labelName);
    LONGS_EQUAL(0x0, m_expression.value);
    LONGS_EQUAL(TYPE_ABSOLUTE, m_expression.type);
    CHECK_TRUE(m_expression.flags & EXPRESSION_FLAG_FORWARD_REFERENCE);
    
    Symbol* pSymbol = Assembler_FindLabel(m_pAssembler, labelName, strlen(labelName));
    CHECK(pSymbol);
    Symbol_LineReferenceEnumStart(pSymbol);
    LineInfo* pLineInfo = Symbol_LineReferenceEnumNext(pSymbol);
    CHECK(pLineInfo);
}

TEST(ExpressionEval, ForwardLabelReferenceAsFirstTerm)
{
    static const char labelName[] = "fwd_label+1";
    m_expression = ExpressionEval(m_pAssembler, labelName);
    LONGS_EQUAL(0x1, m_expression.value);
    LONGS_EQUAL(TYPE_ABSOLUTE, m_expression.type);
    CHECK_TRUE(m_expression.flags & EXPRESSION_FLAG_FORWARD_REFERENCE);
}

TEST(ExpressionEval, ForwardLabelReferenceAsSecondTerm)
{
    static const char labelName[] = "1+fwd_label";
    m_expression = ExpressionEval(m_pAssembler, labelName);
    LONGS_EQUAL(0x1, m_expression.value);
    LONGS_EQUAL(TYPE_ABSOLUTE, m_expression.type);
    CHECK_TRUE(m_expression.flags & EXPRESSION_FLAG_FORWARD_REFERENCE);
}

TEST(ExpressionEval, BackwardLabelReference)
{
    static const char labelName[] = "backLabel";
    setupAssemblerModule("backLabel equ $a55a\n");
    m_expression = ExpressionEval(m_pAssembler, labelName);
    LONGS_EQUAL(0xa55a, m_expression.value);
    LONGS_EQUAL(TYPE_ABSOLUTE, m_expression.type);
    CHECK_FALSE(m_expression.flags & EXPRESSION_FLAG_FORWARD_REFERENCE);

    Symbol* pSymbol = Assembler_FindLabel(m_pAssembler, labelName, strlen(labelName));
    CHECK(pSymbol);
    Symbol_LineReferenceEnumStart(pSymbol);
    LineInfo* pLineInfo = Symbol_LineReferenceEnumNext(pSymbol);
    POINTERS_EQUAL(NULL, pLineInfo);
}

TEST(ExpressionEval, FailAllocationOnForwardLabelReference)
{
    MallocFailureInject_FailAllocation(1);
    m_expression = ExpressionEval(m_pAssembler, "fwd_label");
    MallocFailureInject_Restore();

    LONGS_EQUAL(0x0, m_expression.value);
    LONGS_EQUAL(TYPE_ABSOLUTE, m_expression.type);
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}
