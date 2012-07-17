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
    #include "AddressingMode.h"
    #include "printfSpy.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(AddressingMode)
{
    AddressingMode m_addressingMode;
    Assembler*     m_pAssembler;
    char           m_buffer[64];
    
    void setup()
    {
        printfSpy_Hook(128);
        memset(&m_addressingMode, 0xff, sizeof(m_addressingMode));
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
    
    void setupAssemblerModule(const char* pAsmSource)
    {
        Assembler_Free(m_pAssembler);
        m_pAssembler = Assembler_CreateFromString(dupe(pAsmSource));
        CHECK(m_pAssembler != NULL);
        Assembler_Run(m_pAssembler);
    }
};


TEST(AddressingMode, ImmediateMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, "#1");
    LONGS_EQUAL(ADDRESSING_MODE_IMMEDIATE, m_addressingMode.mode);
    LONGS_EQUAL(TYPE_IMMEDIATE, m_addressingMode.expression.type);
    LONGS_EQUAL(1, m_addressingMode.expression.value);
}

TEST(AddressingMode, AbsoluteMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, "65535");
    LONGS_EQUAL(ADDRESSING_MODE_ABSOLUTE, m_addressingMode.mode);
    LONGS_EQUAL(TYPE_ABSOLUTE, m_addressingMode.expression.type);
    LONGS_EQUAL(65535, m_addressingMode.expression.value);
}

TEST(AddressingMode, ZeroPageAbsoluteMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, "255");
    LONGS_EQUAL(ADDRESSING_MODE_ABSOLUTE, m_addressingMode.mode);
    LONGS_EQUAL(TYPE_ZEROPAGE, m_addressingMode.expression.type);
    LONGS_EQUAL(255, m_addressingMode.expression.value);
}

TEST(AddressingMode, ImpliedMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, NULL);
    LONGS_EQUAL(ADDRESSING_MODE_IMPLIED, m_addressingMode.mode);
}

TEST(AddressingMode, AbsoluteIndexedXMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, "256,X");
    LONGS_EQUAL(ADDRESSING_MODE_ABSOLUTE_INDEXED_X, m_addressingMode.mode);
    LONGS_EQUAL(TYPE_ABSOLUTE, m_addressingMode.expression.type);
    LONGS_EQUAL(256, m_addressingMode.expression.value);
}

TEST(AddressingMode, AbsoluteIndexedXModeWithLowercase)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, "256,x");
    LONGS_EQUAL(ADDRESSING_MODE_ABSOLUTE_INDEXED_X, m_addressingMode.mode);
    LONGS_EQUAL(TYPE_ABSOLUTE, m_addressingMode.expression.type);
    LONGS_EQUAL(256, m_addressingMode.expression.value);
}

TEST(AddressingMode, ZeroPageAbsoluteIndexedXMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, "255,x");
    LONGS_EQUAL(ADDRESSING_MODE_ABSOLUTE_INDEXED_X, m_addressingMode.mode);
    LONGS_EQUAL(TYPE_ZEROPAGE, m_addressingMode.expression.type);
    LONGS_EQUAL(255, m_addressingMode.expression.value);
}

TEST(AddressingMode, AbsoluteIndexedYMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, "256,Y");
    LONGS_EQUAL(ADDRESSING_MODE_ABSOLUTE_INDEXED_Y, m_addressingMode.mode);
    LONGS_EQUAL(TYPE_ABSOLUTE, m_addressingMode.expression.type);
    LONGS_EQUAL(256, m_addressingMode.expression.value);
}

TEST(AddressingMode, AbsoluteIndexedYModeWithLowercase)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, "256,y");
    LONGS_EQUAL(ADDRESSING_MODE_ABSOLUTE_INDEXED_Y, m_addressingMode.mode);
    LONGS_EQUAL(TYPE_ABSOLUTE, m_addressingMode.expression.type);
    LONGS_EQUAL(256, m_addressingMode.expression.value);
}

TEST(AddressingMode, ZeroPageAbsoluteIndexedYMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, "255,y");
    LONGS_EQUAL(ADDRESSING_MODE_ABSOLUTE_INDEXED_Y, m_addressingMode.mode);
    LONGS_EQUAL(TYPE_ZEROPAGE, m_addressingMode.expression.type);
    LONGS_EQUAL(255, m_addressingMode.expression.value);
}

TEST(AddressingMode, InvalidAbsoluteIndexedAddressingMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, "255,Z");
    LONGS_EQUAL(ADDRESSING_MODE_INVALID, m_addressingMode.mode);
}
