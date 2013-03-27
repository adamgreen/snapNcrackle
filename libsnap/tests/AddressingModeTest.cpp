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

#include <string.h>
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
    SizedString    m_string;
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

    SizedString* toSizedString(const char* pString)
    {
        m_string = SizedString_InitFromString(pString);
        return &m_string;
    }

    void setupAssemblerModule(const char* pAsmSource)
    {
        Assembler_Free(m_pAssembler);
        m_pAssembler = Assembler_CreateFromString(dupe(pAsmSource), NULL);
        CHECK(m_pAssembler != NULL);
        Assembler_Run(m_pAssembler);
    }
    
    void validateInvalidArgumentExceptionAndMessage(const char* pExpectedFailureMessage)
    {
        STRCMP_EQUAL(pExpectedFailureMessage, printfSpy_GetLastErrorOutput());
        validateInvalidArgumentExceptionThrown();
    }

    void validateInvalidArgumentExceptionThrown()
    {
        LONGS_EQUAL(invalidArgumentException, getExceptionCode());
        clearExceptionCode();
    }
    
    void validateAddressingMode(AddressingModes expectedMode, ExpressionType expectedType, unsigned short expectedValue)
    {
        LONGS_EQUAL(expectedMode, m_addressingMode.mode);
        LONGS_EQUAL(expectedType, m_addressingMode.expression.type);
        LONGS_EQUAL(expectedValue, m_addressingMode.expression.value);
    }
};


TEST(AddressingMode, ImmediateMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("#1"));
    validateAddressingMode(ADDRESSING_MODE_IMMEDIATE, TYPE_IMMEDIATE, 1);
}

TEST(AddressingMode, ImmediateModeValueFor16BitValue)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("#256"));
    validateAddressingMode(ADDRESSING_MODE_IMMEDIATE, TYPE_IMMEDIATE, 256);
}

TEST(AddressingMode, ImmediateModeWithComment)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("#1 Comment"));
    validateAddressingMode(ADDRESSING_MODE_IMMEDIATE, TYPE_IMMEDIATE, 1);
}

TEST(AddressingMode, AbsoluteMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("65535"));
    validateAddressingMode(ADDRESSING_MODE_ABSOLUTE, TYPE_ABSOLUTE, 65535);
}

TEST(AddressingMode, AbsoluteModeWithComment)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("65535 Comment"));
    validateAddressingMode(ADDRESSING_MODE_ABSOLUTE, TYPE_ABSOLUTE, 65535);
}

TEST(AddressingMode, InvalidAbsoluteModeValue)
{
    __try_and_catch( m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("+65535")) );
    validateInvalidArgumentExceptionAndMessage("filename:0: error: Unexpected prefix in '+65535' expression.\n");
}

TEST(AddressingMode, ZeroPageAbsoluteMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("255"));
    validateAddressingMode(ADDRESSING_MODE_ABSOLUTE, TYPE_ZEROPAGE, 255);
}

TEST(AddressingMode, ZeroPageAbsoluteModeWithComment)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("255 Comment"));
    validateAddressingMode(ADDRESSING_MODE_ABSOLUTE, TYPE_ZEROPAGE, 255);
}

TEST(AddressingMode, ImpliedMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString(NULL));
    LONGS_EQUAL(ADDRESSING_MODE_IMPLIED, m_addressingMode.mode);
}

TEST(AddressingMode, AbsoluteIndexedXMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("256,X"));
    validateAddressingMode(ADDRESSING_MODE_ABSOLUTE_INDEXED_X, TYPE_ABSOLUTE, 256);
}

TEST(AddressingMode, AbsoluteIndexedXModeWithLowercase)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("256,x"));
    validateAddressingMode(ADDRESSING_MODE_ABSOLUTE_INDEXED_X, TYPE_ABSOLUTE, 256);
}

TEST(AddressingMode, AbsoluteIndexedXModeWithComment)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("256,x Comment"));
    validateAddressingMode(ADDRESSING_MODE_ABSOLUTE_INDEXED_X, TYPE_ABSOLUTE, 256);
}

TEST(AddressingMode, ZeroPageAbsoluteIndexedXMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("255,x"));
    validateAddressingMode(ADDRESSING_MODE_ABSOLUTE_INDEXED_X, TYPE_ZEROPAGE, 255);
}

TEST(AddressingMode, ZeroPageAbsoluteIndexedXModeWithComment)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("255,x Comment"));
    validateAddressingMode(ADDRESSING_MODE_ABSOLUTE_INDEXED_X, TYPE_ZEROPAGE, 255);
}

TEST(AddressingMode, InvalidAbsoluteIndexedXModeValue)
{
    __try_and_catch( m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("+256,X")) );
    validateInvalidArgumentExceptionAndMessage("filename:0: error: Unexpected prefix in '+256' expression.\n");
}

TEST(AddressingMode, AbsoluteIndexedYMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("256,Y"));
    validateAddressingMode(ADDRESSING_MODE_ABSOLUTE_INDEXED_Y, TYPE_ABSOLUTE, 256);
}

TEST(AddressingMode, AbsoluteIndexedYModeWithLowercase)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("256,y"));
    validateAddressingMode(ADDRESSING_MODE_ABSOLUTE_INDEXED_Y, TYPE_ABSOLUTE, 256);
}

TEST(AddressingMode, AbsoluteIndexedYModeWithComment)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("256,y Comment"));
    validateAddressingMode(ADDRESSING_MODE_ABSOLUTE_INDEXED_Y, TYPE_ABSOLUTE, 256);
}

TEST(AddressingMode, ZeroPageAbsoluteIndexedYMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("255,y"));
    validateAddressingMode(ADDRESSING_MODE_ABSOLUTE_INDEXED_Y, TYPE_ZEROPAGE, 255);
}

TEST(AddressingMode, ZeroPageAbsoluteIndexedYModeWithComment)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("255,y Comment"));
    validateAddressingMode(ADDRESSING_MODE_ABSOLUTE_INDEXED_Y, TYPE_ZEROPAGE, 255);
}

TEST(AddressingMode, InvalidAbsoluteIndexedYModeValue)
{
    __try_and_catch( m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("+256,Y")) );
    validateInvalidArgumentExceptionAndMessage("filename:0: error: Unexpected prefix in '+256' expression.\n");
}

TEST(AddressingMode, InvalidAbsoluteIndexedAddressingMode)
{
    __try_and_catch( m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("255,Z")) );
    validateInvalidArgumentExceptionAndMessage("filename:0: error: 'Z' isn't a valid index register for this addressing mode.\n");
}

TEST(AddressingMode, ZeroPageIndexedIndirectMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(255,X)"));
    validateAddressingMode(ADDRESSING_MODE_INDEXED_INDIRECT, TYPE_ZEROPAGE, 255);
}

TEST(AddressingMode, ZeroPageIndexedIndirectModeLowerCase)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(0,x)"));
    validateAddressingMode(ADDRESSING_MODE_INDEXED_INDIRECT, TYPE_ZEROPAGE, 0);
}

TEST(AddressingMode, ZeroPageIndexedIndirectModeWithComment)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(0,x) Comment"));
    validateAddressingMode(ADDRESSING_MODE_INDEXED_INDIRECT, TYPE_ZEROPAGE, 0);
}

TEST(AddressingMode, AbsoluteIndexedIndirectMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(256,X)"));
    validateAddressingMode(ADDRESSING_MODE_INDEXED_INDIRECT, TYPE_ABSOLUTE, 256);
}

TEST(AddressingMode, AbsoluteIndexedIndirectModeLowerCase)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(65535,x)"));
    validateAddressingMode(ADDRESSING_MODE_INDEXED_INDIRECT, TYPE_ABSOLUTE, 65535);
}

TEST(AddressingMode, AbsoluteIndexedIndirectModeWithComment)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(65535,x) Comment"));
    validateAddressingMode(ADDRESSING_MODE_INDEXED_INDIRECT, TYPE_ABSOLUTE, 65535);
}

TEST(AddressingMode, InvalidIndexedIndirectModeValue)
{
    __try_and_catch( m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(-,x)")) );
    validateInvalidArgumentExceptionAndMessage("filename:0: error: Unexpected prefix in '' expression.\n");
}

TEST(AddressingMode, InvalidIndexedIndirectModeWithMissingCloseParen)
{
    __try_and_catch( m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(256,x")) );
    validateInvalidArgumentExceptionAndMessage("filename:0: error: '(256,x' doesn't represent a known addressing mode.\n");
}

TEST(AddressingMode, InvalidIndexedIndirectModeRegister)
{
    __try_and_catch( m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(255,Y)")) );
    validateInvalidArgumentExceptionAndMessage("filename:0: error: 'Y' isn't a valid index register for this addressing mode.\n");
}

TEST(AddressingMode, IndirectIndexedMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(0),Y"));
    validateAddressingMode(ADDRESSING_MODE_INDIRECT_INDEXED, TYPE_ZEROPAGE, 0);
}

TEST(AddressingMode, IndirectIndexedModeWithComment)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(0),Y Comment"));
    validateAddressingMode(ADDRESSING_MODE_INDIRECT_INDEXED, TYPE_ZEROPAGE, 0);
}

TEST(AddressingMode, InvalidIndirectIndexedModeValue)
{
    __try_and_catch( m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(+0),Y")) );
    validateInvalidArgumentExceptionAndMessage("filename:0: error: Unexpected prefix in '+0' expression.\n");
}

TEST(AddressingMode, InvalidIndirectIndexedModeNotInZeroPage)
{
    __try_and_catch( m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(256),Y")) );
    validateInvalidArgumentExceptionAndMessage("filename:0: error: '256' isn't in page zero as required for indirect indexed addressing.\n");
}

TEST(AddressingMode, InvalidIndirectIndexedModeRegister)
{
    __try_and_catch( m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(0),X")) );
    validateInvalidArgumentExceptionAndMessage("filename:0: error: 'X' isn't a valid index register for this addressing mode.\n");
}

TEST(AddressingMode, ZeroPageIndirectMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(255)"));
    validateAddressingMode(ADDRESSING_MODE_INDIRECT, TYPE_ZEROPAGE, 255);
}

TEST(AddressingMode, ZeroPageIndirectModeWithComment)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(255) Comment"));
    validateAddressingMode(ADDRESSING_MODE_INDIRECT, TYPE_ZEROPAGE, 255);
}

TEST(AddressingMode, AbsolutePageIndirectMode)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(256)"));
    validateAddressingMode(ADDRESSING_MODE_INDIRECT, TYPE_ABSOLUTE, 256);
}

TEST(AddressingMode, AbsolutePageIndirectModeWithComment)
{
    m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(256) Comment"));
    validateAddressingMode(ADDRESSING_MODE_INDIRECT, TYPE_ABSOLUTE, 256);
}

TEST(AddressingMode, InvalidIndirectModeValue)
{
    __try_and_catch( m_addressingMode = AddressingMode_Eval(m_pAssembler, toSizedString("(+0)")) );
    validateInvalidArgumentExceptionAndMessage("filename:0: error: Unexpected prefix in '+0' expression.\n");
}
