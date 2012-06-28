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
#include <string.h>
extern "C"
{
    #include "ParseLine.h"
    #include "try_catch.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"

TEST_GROUP(LineParser)
{
    ParsedLine  m_parsedLine;
    char        m_buffer[128];

    void setup()
    {
        clearExceptionCode();
        memset(&m_parsedLine, 0xff, sizeof(m_parsedLine));
    }

    void teardown()
    {
        LONGS_EQUAL(noException, getExceptionCode());
    }

    char* dupe(const char* pString)
    {
        strcpy(m_buffer, pString);
        return m_buffer;
    }
};


TEST(LineParser, EmptyLine)
{
    ParseLine(&m_parsedLine, dupe(""));
    POINTERS_EQUAL(NULL, m_parsedLine.pLabel);
    POINTERS_EQUAL(NULL, m_parsedLine.pOperator);
    POINTERS_EQUAL(NULL, m_parsedLine.pOperands);
}

TEST(LineParser, FullLineComment)
{
    static const char* pCommentLine = "* Comment";
    
    ParseLine(&m_parsedLine, dupe(pCommentLine));
    POINTERS_EQUAL(NULL, m_parsedLine.pLabel);
    POINTERS_EQUAL(NULL, m_parsedLine.pOperator);
    POINTERS_EQUAL(NULL, m_parsedLine.pOperands);
}

TEST(LineParser, AlternativeFullLineComment)
{
    static const char* pCommentLine = "; Other Comment";
    
    ParseLine(&m_parsedLine, dupe(pCommentLine));
    POINTERS_EQUAL(NULL, m_parsedLine.pLabel);
    POINTERS_EQUAL(NULL, m_parsedLine.pOperator);
    POINTERS_EQUAL(NULL, m_parsedLine.pOperands);
}

TEST(LineParser, LabelOnly)
{
    static const char* pLabelLine = ":Label";
    
    ParseLine(&m_parsedLine, dupe(pLabelLine));
    STRCMP_EQUAL(pLabelLine, m_parsedLine.pLabel);
    POINTERS_EQUAL(NULL, m_parsedLine.pOperator);
    POINTERS_EQUAL(NULL, m_parsedLine.pOperands);
}

TEST(LineParser, OperatorOnly)
{
    ParseLine(&m_parsedLine, dupe(" INY"));
    STRCMP_EQUAL("INY", m_parsedLine.pOperator);
    POINTERS_EQUAL(NULL, m_parsedLine.pLabel);
    POINTERS_EQUAL(NULL, m_parsedLine.pOperands);
}

TEST(LineParser, LabelAndOperator)
{
    ParseLine(&m_parsedLine, dupe(":Label EQU"));
    STRCMP_EQUAL(":Label", m_parsedLine.pLabel);
    STRCMP_EQUAL("EQU", m_parsedLine.pOperator);
    POINTERS_EQUAL(NULL, m_parsedLine.pOperands);
}

TEST(LineParser, OperatorAndOperands)
{
    ParseLine(&m_parsedLine, dupe(" ORG $800"));
    POINTERS_EQUAL(NULL, m_parsedLine.pLabel);
    STRCMP_EQUAL("ORG", m_parsedLine.pOperator);
    STRCMP_EQUAL("$800", m_parsedLine.pOperands);
}

TEST(LineParser, LabelOperatorAndOperands)
{
    ParseLine(&m_parsedLine, dupe("org EQU $C00"));
    STRCMP_EQUAL("org", m_parsedLine.pLabel);
    STRCMP_EQUAL("EQU", m_parsedLine.pOperator);
    STRCMP_EQUAL("$C00", m_parsedLine.pOperands);
}

TEST(LineParser, OperatorOperandsComments)
{
    ParseLine(&m_parsedLine, dupe(" ORG $C00 Program should start here"));
    POINTERS_EQUAL(NULL, m_parsedLine.pLabel);
    STRCMP_EQUAL("ORG", m_parsedLine.pOperator);
    STRCMP_EQUAL("$C00", m_parsedLine.pOperands);
}

TEST(LineParser, LabelOperatorOperandsComments)
{
    ParseLine(&m_parsedLine, dupe("org EQU $C00 Program should start here"));
    STRCMP_EQUAL("org", m_parsedLine.pLabel);
    STRCMP_EQUAL("EQU", m_parsedLine.pOperator);
    STRCMP_EQUAL("$C00", m_parsedLine.pOperands);
}

TEST(LineParser, LabelComments)
{
    ParseLine(&m_parsedLine, dupe("entry ;Entry point of program"));
    STRCMP_EQUAL("entry", m_parsedLine.pLabel);
    POINTERS_EQUAL(NULL, m_parsedLine.pOperator);
    POINTERS_EQUAL(NULL, m_parsedLine.pOperands);
}

TEST(LineParser, OperatorComments)
{
    ParseLine(&m_parsedLine, dupe(" INY ; Increment Y"));
    POINTERS_EQUAL(NULL, m_parsedLine.pLabel);
    STRCMP_EQUAL("INY", m_parsedLine.pOperator);
    POINTERS_EQUAL(NULL, m_parsedLine.pOperands);
}

TEST(LineParser, MultipleSpacesBeforeOperator)
{
    ParseLine(&m_parsedLine, dupe("    INY"));
    POINTERS_EQUAL(NULL, m_parsedLine.pLabel);
    STRCMP_EQUAL("INY", m_parsedLine.pOperator);
    POINTERS_EQUAL(NULL, m_parsedLine.pOperands);
}

TEST(LineParser, MultipleSpacesBetweenLabelOperator)
{
    ParseLine(&m_parsedLine, dupe("Start    INY"));
    STRCMP_EQUAL("Start", m_parsedLine.pLabel);
    STRCMP_EQUAL("INY", m_parsedLine.pOperator);
    POINTERS_EQUAL(NULL, m_parsedLine.pOperands);
}

TEST(LineParser, MultipleSpacesBetweenOperatorOperands)
{
    ParseLine(&m_parsedLine, dupe(" BIT    $F0"));
    POINTERS_EQUAL(NULL, m_parsedLine.pLabel);
    STRCMP_EQUAL("BIT", m_parsedLine.pOperator);
    STRCMP_EQUAL("$F0", m_parsedLine.pOperands);
}

TEST(LineParser, TabForWhitespace)
{
    ParseLine(&m_parsedLine, dupe("\tBIT\t$F0"));
    POINTERS_EQUAL(NULL, m_parsedLine.pLabel);
    STRCMP_EQUAL("BIT", m_parsedLine.pOperator);
    STRCMP_EQUAL("$F0", m_parsedLine.pOperands);
}
