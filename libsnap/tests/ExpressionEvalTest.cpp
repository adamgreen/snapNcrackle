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
    SizedString  m_string;
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

    SizedString* toSizedString(const char* pString)
    {
        m_string = SizedString_InitFromString(pString);
        return &m_string;
    }
    
    void validateFailureMessageAndThrownException(const char* pExpectedErrorMessage, int expectedExceptionCode)
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
        m_pAssembler = Assembler_CreateFromString(dupe(pAsmSource), NULL);
        CHECK(m_pAssembler != NULL);
        Assembler_Run(m_pAssembler);
    }
    
    void validateExpression(ExpressionType expectedType, unsigned short expectedValue)
    {
        LONGS_EQUAL(expectedType, m_expression.type);
        LONGS_EQUAL(expectedValue, m_expression.value);
    }
};


TEST(ExpressionEval, EvaluateHexLiteral)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("$AF09"));
    validateExpression(TYPE_ABSOLUTE, 0xaf09);
}

TEST(ExpressionEval, EvaluateInvalidHexDigit)
{
    __try_and_catch( m_expression = ExpressionEval(m_pAssembler, toSizedString("$AG")) );
    validateFailureMessageAndThrownException("filename:0: error: 'G' is unexpected operator.\n", invalidArgumentException);
}

TEST(ExpressionEval, EvaluateHexValueTooLong)
{
    __try_and_catch( m_expression = ExpressionEval(m_pAssembler, toSizedString("$12345")) );
    validateFailureMessageAndThrownException("filename:0: error: Hexadecimal number '$12345' doesn't fit in 16-bits.\n", invalidArgumentException);
}

TEST(ExpressionEval, EvaluateHexImmediate)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("#$60"));
    validateExpression(TYPE_IMMEDIATE, 0x60);
}

TEST(ExpressionEval, EvaluateBinaryLiteral)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("%1010111100001001"));
    validateExpression(TYPE_ABSOLUTE, 0xaf09);
}

TEST(ExpressionEval, EvaluateInvalidBinaryDigit)
{
    __try_and_catch( m_expression = ExpressionEval(m_pAssembler, toSizedString("%12")) );
    validateFailureMessageAndThrownException("filename:0: error: '2' is unexpected operator.\n", invalidArgumentException);
}

TEST(ExpressionEval, EvaluateBinaryValueTooLong)
{
    __try_and_catch( m_expression = ExpressionEval(m_pAssembler, toSizedString("%11110000111100001")) );
    validateFailureMessageAndThrownException("filename:0: error: Binary number '%11110000111100001' doesn't fit in 16-bits.\n", invalidArgumentException);
}

TEST(ExpressionEval, EvaluateBinaryImmediate)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("#%11110000"));
    validateExpression(TYPE_IMMEDIATE, 0xF0);
}

TEST(ExpressionEval, EvaluateDecimalLiteral)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("65535"));
    validateExpression(TYPE_ABSOLUTE, 0xffff);
}

TEST(ExpressionEval, EvaluateInvalidDecimalDigit)
{
    __try_and_catch( m_expression = ExpressionEval(m_pAssembler, toSizedString("1f")) );
    validateFailureMessageAndThrownException("filename:0: error: 'f' is unexpected operator.\n", invalidArgumentException);
}

TEST(ExpressionEval, EvaluateDecimalValueTooLong)
{
    __try_and_catch( m_expression = ExpressionEval(m_pAssembler, toSizedString("65536")) );
    validateFailureMessageAndThrownException("filename:0: error: Decimal number '65536' doesn't fit in 16-bits.\n", invalidArgumentException);
}

TEST(ExpressionEval, EvaluateDecimalImmediate)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("#255"));
    validateExpression(TYPE_IMMEDIATE, 0xFF);
}

TEST(ExpressionEval, ImmediateValueLargerThan8Bits)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("#$100"));
    validateExpression(TYPE_IMMEDIATE, 0x100);
}

TEST(ExpressionEval, ImmediateValueLargerThan8BitsWithLessThanPrefix)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("#<$100"));
    validateExpression(TYPE_IMMEDIATE, 0x100 & 0xff);
}

TEST(ExpressionEval, ImmediateValueLargerThan8BitsWithGreaterThanPrefix)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("#>$100"));
    validateExpression(TYPE_IMMEDIATE, 0x100 >> 8);
}

TEST(ExpressionEval, ImmediateExpressionWithHighByteExtractionOnLongerExpression)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("#>$D600-$400"));
    validateExpression(TYPE_IMMEDIATE, (0xD600-0x400) >> 8);
}

TEST(ExpressionEval, ImmediateValueLargerThan8BitsWithCaretPrefix)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("#^$100"));
    validateExpression(TYPE_IMMEDIATE, 0x0100 >> 8);
}

TEST(ExpressionEval, UseOfLessThanPrefixOnNonImmediateValue)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("<$100"));
    validateExpression(TYPE_ABSOLUTE, 0x0100 & 0xFF);
}

TEST(ExpressionEval, UseOfGreaterThanPrefixOnNonImmediateValue)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString(">$100"));
    validateExpression(TYPE_ABSOLUTE, 0x0100 >> 8);
}

TEST(ExpressionEval, UseOfCaretPrefixOnNonImmediateValue)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("^$100"));
    validateExpression(TYPE_ABSOLUTE, 0x0100 >> 8);
}

TEST(ExpressionEval, EvaluateSingleQuotedASCII)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("#'a'"));
    validateExpression(TYPE_IMMEDIATE, 'a');
}

TEST(ExpressionEval, EvaluateSingleQuotedPrefixOnlyASCII)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("#'a+1"));
    validateExpression(TYPE_IMMEDIATE, 'a'+1);
}

TEST(ExpressionEval, EvaluateDoubleQuotedASCII)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("#\"a\""));
    validateExpression(TYPE_IMMEDIATE, 'a' | 0x80);
}

TEST(ExpressionEval, EvaluateDoubleQuotedPrefixOnlyASCII)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("#\"a+1"));
    validateExpression(TYPE_IMMEDIATE, ('a' | 0x80)+1);
}

TEST(ExpressionEval, AsteriskInExpression)
{
    setupAssemblerModule(" org $800\n");
    m_expression = ExpressionEval(m_pAssembler, toSizedString("*"));
    validateExpression(TYPE_ABSOLUTE, 0x800);
}

TEST(ExpressionEval, EvaluateAddition)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("2+3"));
    validateExpression(TYPE_ZEROPAGE, 2+3);
}

TEST(ExpressionEval, EvaluateSubtraction)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("3-2"));
    validateExpression(TYPE_ZEROPAGE, 3-2);
}

TEST(ExpressionEval, EvaluateMultiplication)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("3*2"));
    validateExpression(TYPE_ZEROPAGE, 3*2);
}

TEST(ExpressionEval, EvaluateDivision)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("3/2"));
    validateExpression(TYPE_ZEROPAGE, 3/2);
}

TEST(ExpressionEval, EvaluateUnarySubtraction)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("-2"));
    validateExpression(TYPE_ABSOLUTE, 0xFFFE);
}

TEST(ExpressionEval, EvaluateUnarySubtractionInMiddleOfExpression)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("2--2"));
    validateExpression(TYPE_ABSOLUTE, 2- -2);
}

TEST(ExpressionEval, EvaluateNoOperatorPrecedenceExample)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("2+3*5"));
    validateExpression(TYPE_ZEROPAGE, (2+3)*5);
}

TEST(ExpressionEval, EvaluateXOR)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("$ff!$80"));
    validateExpression(TYPE_ZEROPAGE, 0x7f);
}

TEST(ExpressionEval, EvaluateOR)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("$00.$80"));
    validateExpression(TYPE_ZEROPAGE, 0x80);
}

TEST(ExpressionEval, EvaluateAND)
{
    m_expression = ExpressionEval(m_pAssembler, toSizedString("$af&$f0"));
    validateExpression(TYPE_ZEROPAGE, 0xaf&0xf0);
}

TEST(ExpressionEval, ForwardLabelReference)
{
    static const char labelName[] = "fwd_label";
    m_expression = ExpressionEval(m_pAssembler, toSizedString(labelName));
    validateExpression(TYPE_ABSOLUTE, 0x0);
    CHECK_TRUE(m_expression.flags & EXPRESSION_FLAG_FORWARD_REFERENCE);
    
    Symbol* pSymbol = Assembler_FindLabel(m_pAssembler, toSizedString(labelName));
    CHECK(pSymbol);
    Symbol_LineReferenceEnumStart(pSymbol);
    LineInfo* pLineInfo = Symbol_LineReferenceEnumNext(pSymbol);
    CHECK(pLineInfo);
}

TEST(ExpressionEval, ForwardLabelReferenceAsFirstTerm)
{
    static const char labelName[] = "fwd_label+1";
    m_expression = ExpressionEval(m_pAssembler, toSizedString(labelName));
    validateExpression(TYPE_ABSOLUTE, 0x1);
    CHECK_TRUE(m_expression.flags & EXPRESSION_FLAG_FORWARD_REFERENCE);
}

TEST(ExpressionEval, ForwardLabelReferenceAsSecondTerm)
{
    static const char labelName[] = "1+fwd_label";
    m_expression = ExpressionEval(m_pAssembler, toSizedString(labelName));
    validateExpression(TYPE_ABSOLUTE, 0x1);
    CHECK_TRUE(m_expression.flags & EXPRESSION_FLAG_FORWARD_REFERENCE);
}

TEST(ExpressionEval, BackwardLabelReference)
{
    static const char labelName[] = "backLabel";
    setupAssemblerModule("backLabel equ $a55a\n");
    m_expression = ExpressionEval(m_pAssembler, toSizedString(labelName));
    validateExpression(TYPE_ABSOLUTE, 0xa55a);
    CHECK_FALSE(m_expression.flags & EXPRESSION_FLAG_FORWARD_REFERENCE);

    Symbol* pSymbol = Assembler_FindLabel(m_pAssembler, toSizedString(labelName));
    CHECK(pSymbol);
    Symbol_LineReferenceEnumStart(pSymbol);
    LineInfo* pLineInfo = Symbol_LineReferenceEnumNext(pSymbol);
    POINTERS_EQUAL(NULL, pLineInfo);
}

TEST(ExpressionEval, FailAllocationOnForwardLabelReference)
{
    MallocFailureInject_FailAllocation(1);
        __try_and_catch( m_expression = ExpressionEval(m_pAssembler, toSizedString("fwd_label")) );
    MallocFailureInject_Restore();

    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}
