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
    #include "ParseCSV.h"
    #include "MallocFailureInject.h"
    #include "util.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(ParseCSV)
{
    ParseCSV* m_pParser;
    char      m_copy[128];
    
    void setup()
    {
        m_pParser = NULL;
    }

    void teardown()
    {
        MallocFailureInject_Restore();
        ParseCSV_Free(m_pParser);
    }
    
    char* copy(const char* pString)
    {
        strcpy(m_copy, pString);
        return m_copy;
    }
    
    void validateOutOfMemoryExceptionThrown()
    {
        LONGS_EQUAL(outOfMemoryException, getExceptionCode());
        clearExceptionCode();
    }
    
    void parseAndValidate(const char* testString, size_t expectedTokenCount, const char** ppExpectedTokens)
    {
        ParseCSV_Parse(m_pParser, copy(testString));
        LONGS_EQUAL(expectedTokenCount - 1, ParseCSV_FieldCount(m_pParser));
        const char** ppFields = ParseCSV_FieldPointers(m_pParser);
        for (size_t i = 0 ; i < expectedTokenCount ; i++)
        {
            STRCMP_EQUAL(ppExpectedTokens[i], ppFields[i]);
        }
    }
};


TEST(ParseCSV, FailAllocationDuringCreate)
{
    MallocFailureInject_FailAllocation(1);
    __try_and_catch( m_pParser = ParseCSV_Create() );
    validateOutOfMemoryExceptionThrown();
}

TEST(ParseCSV, ParseNullPointer)
{
    m_pParser = ParseCSV_Create();
    CHECK(m_pParser != NULL);
    ParseCSV_Parse(m_pParser, NULL);
    LONGS_EQUAL(0, ParseCSV_FieldCount(m_pParser));
    POINTERS_EQUAL(NULL, *ParseCSV_FieldPointers(m_pParser));
}

TEST(ParseCSV, ParseBlankLine)
{
    m_pParser = ParseCSV_Create();

    static const char* ppExpectedFields[] = { NULL };
    parseAndValidate("", ARRAYSIZE(ppExpectedFields), ppExpectedFields);
}

TEST(ParseCSV, ParseOneCharacterInLine)
{
    m_pParser = ParseCSV_Create();

    static const char* ppExpectedFields[] = { "a", NULL };
    parseAndValidate("a", ARRAYSIZE(ppExpectedFields), ppExpectedFields);
}

TEST(ParseCSV, ParseOneShortTokenAndEmptyToken)
{
    m_pParser = ParseCSV_Create();

    static const char* ppExpectedFields[] = { "a", "", NULL };
    parseAndValidate("a,", ARRAYSIZE(ppExpectedFields), ppExpectedFields);
}

TEST(ParseCSV, ParseTwoTokens)
{
    m_pParser = ParseCSV_Create();
    static const char* ppExpectedFields[] = { "foo", "bar", NULL };
    parseAndValidate("foo,bar", ARRAYSIZE(ppExpectedFields), ppExpectedFields);
}

TEST(ParseCSV, ParseTwoStrings)
{
    m_pParser = ParseCSV_Create();

    static const char* ppExpectedFields1[] = { "token1", NULL };
    parseAndValidate("token1", ARRAYSIZE(ppExpectedFields1), ppExpectedFields1);

    static const char* ppExpectedFields2[] = { "foo", "bar", NULL };
    parseAndValidate("foo,bar", ARRAYSIZE(ppExpectedFields2), ppExpectedFields2);
}

TEST(ParseCSV, FailAllocationOnParse)
{
    m_pParser = ParseCSV_Create();
    MallocFailureInject_FailAllocation(1);
    __try_and_catch( ParseCSV_Parse(m_pParser, copy("a")) );
    validateOutOfMemoryExceptionThrown();
}

TEST(ParseCSV, ParseTwoTokensWithCustomSeparator)
{
    m_pParser = ParseCSV_CreateWithCustomSeparator(';');
    static const char* ppExpectedFields[] = { "foo", "bar", NULL };
    parseAndValidate("foo;bar", ARRAYSIZE(ppExpectedFields), ppExpectedFields);
}
