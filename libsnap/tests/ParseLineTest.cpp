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
    SizedString m_string;

    void setup()
    {
        clearExceptionCode();
        memset(&m_parsedLine, 0xff, sizeof(m_parsedLine));
    }

    void teardown()
    {
        LONGS_EQUAL(noException, getExceptionCode());
    }

    const SizedString* dupe(const char* pString)
    {
        m_string = SizedString_InitFromString(pString);
        return &m_string;
    }
    
    void validateParsedLine(const char* pLabel, const char* pOperator, const char* pOperands)
    {
        if (!pLabel)
        {
            LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.label));
        }
        else
        {
            LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.label, pLabel));
        }
            
        if (!pOperator)
        {
            LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.op));
        }
        else
        {
            LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.op, pOperator));
        }
        
        if (!pOperands)
        {
            LONGS_EQUAL(0, SizedString_strlen(&m_parsedLine.operands));
        }
        else
        {
            LONGS_EQUAL(0, SizedString_strcmp(&m_parsedLine.operands, pOperands));
        }
    }
};


TEST(LineParser, EmptyLine)
{
    ParseLine(&m_parsedLine, dupe(""));
    validateParsedLine(NULL, NULL, NULL);
}

TEST(LineParser, FullLineComment)
{
    static const char* pCommentLine = "* Comment";
    
    ParseLine(&m_parsedLine, dupe(pCommentLine));
    validateParsedLine(NULL, NULL, NULL);
}

TEST(LineParser, AlternativeFullLineComment)
{
    static const char* pCommentLine = "; Other Comment";
    
    ParseLine(&m_parsedLine, dupe(pCommentLine));
    validateParsedLine(NULL, NULL, NULL);
}

TEST(LineParser, LabelOnly)
{
    static const char* pLabelLine = ":Label";
    
    ParseLine(&m_parsedLine, dupe(pLabelLine));
    validateParsedLine(pLabelLine, NULL, NULL);
}

TEST(LineParser, OperatorOnly)
{
    ParseLine(&m_parsedLine, dupe(" INY"));
    validateParsedLine(NULL, "INY", NULL);
}

TEST(LineParser, LabelAndOperator)
{
    ParseLine(&m_parsedLine, dupe(":Label EQU"));
    validateParsedLine(":Label", "EQU", NULL);
}

TEST(LineParser, OperatorAndOperands)
{
    ParseLine(&m_parsedLine, dupe(" ORG $800"));
    validateParsedLine(NULL, "ORG", "$800");
}

TEST(LineParser, LabelOperatorAndOperands)
{
    ParseLine(&m_parsedLine, dupe("org EQU $C00"));
    validateParsedLine("org", "EQU", "$C00");
}

TEST(LineParser, OperatorOperandsComments)
{
    ParseLine(&m_parsedLine, dupe(" ORG $C00 Program should start here"));
    validateParsedLine(NULL, "ORG", "$C00 Program should start here");
}

TEST(LineParser, LabelOperatorOperandsComments)
{
    ParseLine(&m_parsedLine, dupe("org EQU $C00 Program should start here"));
    validateParsedLine("org", "EQU", "$C00 Program should start here");
}

TEST(LineParser, OperatorOperandsSemicolonComment)
{
    ParseLine(&m_parsedLine, dupe(" ORG $C00 ; Program should start here"));
    validateParsedLine(NULL, "ORG", "$C00 ");
}

TEST(LineParser, LabelOperatorOperandsSemicolonComment)
{
    ParseLine(&m_parsedLine, dupe("org EQU $C00 ; Program should start here"));
    validateParsedLine("org", "EQU", "$C00 ");
}

TEST(LineParser, LabelComments)
{
    ParseLine(&m_parsedLine, dupe("entry ;Entry point of program"));
    validateParsedLine("entry", NULL, NULL);
}

TEST(LineParser, OperatorComments)
{
    ParseLine(&m_parsedLine, dupe(" INY ; Increment Y"));
    validateParsedLine(NULL, "INY", NULL);
}

TEST(LineParser, MultipleSpacesBeforeOperator)
{
    ParseLine(&m_parsedLine, dupe("    INY"));
    validateParsedLine(NULL, "INY", NULL);
}

TEST(LineParser, MultipleSpacesBetweenLabelOperator)
{
    ParseLine(&m_parsedLine, dupe("Start    INY"));
    validateParsedLine("Start", "INY", NULL);
}

TEST(LineParser, MultipleSpacesBetweenOperatorOperands)
{
    ParseLine(&m_parsedLine, dupe(" BIT    $F0"));
    validateParsedLine(NULL, "BIT", "$F0");
}

TEST(LineParser, TabForWhitespace)
{
    ParseLine(&m_parsedLine, dupe("\tBIT\t$F0"));
    validateParsedLine(NULL, "BIT", "$F0");
}

TEST(LineParser, SpaceBetweenSingleQuotesInOperand)
{
    ParseLine(&m_parsedLine, dupe(" LDA #' '"));
    validateParsedLine(NULL, "LDA", "#' '");
}

TEST(LineParser, SpaceBetweenDoubleQuoteInOperand)
{
    ParseLine(&m_parsedLine, dupe(" LDA #\" \""));
    validateParsedLine(NULL, "LDA", "#\" \"");
}

TEST(LineParser, SpaceAfterSingleQuotesInOperand)
{
    ParseLine(&m_parsedLine, dupe(" LDA #' +1"));
    validateParsedLine(NULL, "LDA", "#' +1");
}

TEST(LineParser, SpaceAfterDoubleQuoteInOperand)
{
    ParseLine(&m_parsedLine, dupe(" LDA #\" +1"));
    validateParsedLine(NULL, "LDA", "#\" +1");
}
