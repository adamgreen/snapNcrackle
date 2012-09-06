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
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.label));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.op));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.operands));
}

TEST(LineParser, FullLineComment)
{
    static const char* pCommentLine = "* Comment";
    
    ParseLine(&m_parsedLine, dupe(pCommentLine));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.label));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.op));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.operands));
}

TEST(LineParser, AlternativeFullLineComment)
{
    static const char* pCommentLine = "; Other Comment";
    
    ParseLine(&m_parsedLine, dupe(pCommentLine));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.label));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.op));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.operands));
}

TEST(LineParser, LabelOnly)
{
    static const char* pLabelLine = ":Label";
    
    ParseLine(&m_parsedLine, dupe(pLabelLine));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.label, pLabelLine));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.op));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.operands));
}

TEST(LineParser, OperatorOnly)
{
    ParseLine(&m_parsedLine, dupe(" INY"));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.op, "INY"));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.label));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.operands));
}

TEST(LineParser, LabelAndOperator)
{
    ParseLine(&m_parsedLine, dupe(":Label EQU"));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.label, ":Label"));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.op, "EQU"));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.operands));
}

TEST(LineParser, OperatorAndOperands)
{
    ParseLine(&m_parsedLine, dupe(" ORG $800"));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.label));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.op, "ORG"));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.operands, "$800"));
}

TEST(LineParser, LabelOperatorAndOperands)
{
    ParseLine(&m_parsedLine, dupe("org EQU $C00"));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.label, "org"));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.op, "EQU"));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.operands, "$C00"));
}

TEST(LineParser, OperatorOperandsComments)
{
    ParseLine(&m_parsedLine, dupe(" ORG $C00 Program should start here"));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.label));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.op, "ORG"));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.operands, "$C00"));
}

TEST(LineParser, LabelOperatorOperandsComments)
{
    ParseLine(&m_parsedLine, dupe("org EQU $C00 Program should start here"));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.label, "org"));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.op, "EQU"));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.operands, "$C00"));
}

TEST(LineParser, LabelComments)
{
    ParseLine(&m_parsedLine, dupe("entry ;Entry point of program"));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.label, "entry"));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.op));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.operands));
}

TEST(LineParser, OperatorComments)
{
    ParseLine(&m_parsedLine, dupe(" INY ; Increment Y"));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.label));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.op, "INY"));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.operands));
}

TEST(LineParser, MultipleSpacesBeforeOperator)
{
    ParseLine(&m_parsedLine, dupe("    INY"));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.label));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.op, "INY"));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.operands));
}

TEST(LineParser, MultipleSpacesBetweenLabelOperator)
{
    ParseLine(&m_parsedLine, dupe("Start    INY"));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.label, "Start"));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.op, "INY"));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.operands));
}

TEST(LineParser, MultipleSpacesBetweenOperatorOperands)
{
    ParseLine(&m_parsedLine, dupe(" BIT    $F0"));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.label));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.op, "BIT"));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.operands, "$F0"));
}

TEST(LineParser, TabForWhitespace)
{
    ParseLine(&m_parsedLine, dupe("\tBIT\t$F0"));
    LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.label));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.op, "BIT"));
    LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.operands, "$F0"));
}
