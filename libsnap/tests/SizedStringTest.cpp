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
    #include "SizedString.h"
    #include "try_catch.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(SizedString)
{
    const char* m_pExpectedString;
    const char* m_pExpectedContent;
    size_t      m_expectedStringLength;
    SizedString m_sizedString;
    char        m_testString[64];
    
    void setup()
    {
        clearExceptionCode();
        memset(&m_sizedString, 0xff, sizeof(m_sizedString));
    }

    void teardown()
    {
        POINTERS_EQUAL(m_pExpectedString, m_sizedString.pString);
        LONGS_EQUAL(m_expectedStringLength, m_sizedString.stringLength);
        LONGS_EQUAL(noException, getExceptionCode());
        CHECK_TRUE(0 == strncmp(m_pExpectedContent, m_sizedString.pString, m_sizedString.stringLength));
    }
    
    void testSizedStringInit(const char* pString, size_t stringLength)
    {
        m_sizedString = SizedString_Init(pString, stringLength);
        m_pExpectedString = pString;
        m_pExpectedContent = pString;
        m_expectedStringLength = stringLength;
    }
    
    void testStringInit(const char* pString)
    {
        m_sizedString = SizedString_InitFromString(pString);
        m_pExpectedString = pString;
        m_pExpectedContent = pString;
        m_expectedStringLength = strlen(pString);
    }
};


TEST(SizedString, EmptySizedInit)
{
    testSizedStringInit("", 0);
}

TEST(SizedString, OneCharSizedInit)
{
    testSizedStringInit("A", 1);
}

TEST(SizedString, TruncateStringInit)
{
    testSizedStringInit("123456789abcdef", 8);
    strcpy(m_testString, "12345678");
    m_pExpectedContent = m_testString;
}

TEST(SizedString, FailTruncateStringInit)
{
    testSizedStringInit("123456789abcdef", 8);
    CHECK_FALSE(0 == strncmp("1234567", m_sizedString.pString, m_sizedString.stringLength));
}

TEST(SizedString, EmptyInit)
{
    testStringInit("");
}

TEST(SizedString, OneCharInit)
{
    testStringInit("A");
}
