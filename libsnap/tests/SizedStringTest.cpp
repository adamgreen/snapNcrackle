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
    
    void testSizedStringInit(const char* pString)
    {
        m_sizedString = SizedString_InitFromString(pString);
        m_pExpectedString = pString;
        m_pExpectedContent = pString;
        m_expectedStringLength = pString ? strlen(pString) : 0;
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

TEST(SizedString, NullStringInit)
{
    testSizedStringInit(NULL);
}

TEST(SizedString, EmptyInit)
{
    testSizedStringInit("");
}

TEST(SizedString, OneCharInit)
{
    testSizedStringInit("A");
}

TEST(SizedString, strchrInEmptyString)
{
    testSizedStringInit("");
    CHECK_TRUE(NULL == SizedString_strchr(&m_sizedString, ' '));
}

TEST(SizedString, strchrOnlyCharInString)
{
    testSizedStringInit("A");
    const char* pResult = SizedString_strchr(&m_sizedString, 'A');
    CHECK_TRUE(pResult != NULL);
    CHECK_TRUE(*pResult == 'A');
}

TEST(SizedString, strchrCaseSensitive)
{
    testSizedStringInit("A");
    POINTERS_EQUAL(NULL, SizedString_strchr(&m_sizedString, 'a'));
}

TEST(SizedString, strchrLastCharInTruncatedString)
{
    testSizedStringInit("123456789", 8);
    const char* pResult = SizedString_strchr(&m_sizedString, '8');
    CHECK_TRUE(pResult != NULL);
    CHECK_TRUE(*pResult == '8');
}

TEST(SizedString, strchrPastLastCharInTruncatedString)
{
    testSizedStringInit("123456789", 8);
    POINTERS_EQUAL(NULL, SizedString_strchr(&m_sizedString, '9'));
}

TEST(SizedString, strcmpMatchWholeString)
{
    testSizedStringInit("Test String");
    LONGS_EQUAL(strcmp("Test String", "Test String"), SizedString_strcmp(&m_sizedString, "Test String"));
}

TEST(SizedString, strcmpMatchWholeTruncatedString)
{
    testSizedStringInit("Test String", 4);
    LONGS_EQUAL(strcmp("Test", "Test"), SizedString_strcmp(&m_sizedString, "Test"));
}

TEST(SizedString, strcmpFailMatchDueToCaseSensitivity)
{
    testSizedStringInit("Test string");
    LONGS_EQUAL(strcmp("Test string", "Test String"), SizedString_strcmp(&m_sizedString, "Test String"));
}

TEST(SizedString, strcmpFailMatchDueToSearchStringBeingTooShort)
{
    testSizedStringInit("Test string");
    LONGS_EQUAL(strcmp("Test string", "Test"), SizedString_strcmp(&m_sizedString, "Test"));
}

TEST(SizedString, strcmpFailMatchDueToSizedStringBeingTooShort)
{
    testSizedStringInit("Test string", 4);
    LONGS_EQUAL(strcmp("Test", "Test string"), SizedString_strcmp(&m_sizedString, "Test string"));
}

TEST(SizedString, strcasecmpMatchWholeStringWithDifferentCase)
{
    testSizedStringInit("Test String");
    LONGS_EQUAL(strcasecmp("Test String", "tEST sTRING"), SizedString_strcasecmp(&m_sizedString, "tEST sTRING"));
}

TEST(SizedString, strcasecmpMatchWholeTruncatedString)
{
    testSizedStringInit("Test String", 4);
    LONGS_EQUAL(strcasecmp("Test", "tEST"), SizedString_strcasecmp(&m_sizedString, "tEST"));
}

TEST(SizedString, strcasecmpFailMatch)
{
    testSizedStringInit("Test string");
    LONGS_EQUAL(strcasecmp("Test string", "Tst"), SizedString_strcasecmp(&m_sizedString, "Tst"));
}

TEST(SizedString, strcasecmpFailMatchDueToSearchStringBeingTooShort)
{
    testSizedStringInit("Test string");
    LONGS_EQUAL(strcasecmp("Test string", "Test"), SizedString_strcasecmp(&m_sizedString, "Test"));
}

TEST(SizedString, strcasecmpFailMatchDueToSizedStringBeingTooShort)
{
    testSizedStringInit("Test string", 4);
    LONGS_EQUAL(strcasecmp("Test", "Test string"), SizedString_strcasecmp(&m_sizedString, "Test string"));
}

TEST(SizedString, splitStringAroundComma)
{
    testSizedStringInit("256,X");
    
    SizedString beforeComma;
    SizedString afterComma;
    SizedString_SplitString(&m_sizedString, ',', &beforeComma, &afterComma);
    LONGS_EQUAL(0, SizedString_strcmp(&beforeComma, "256"));
    LONGS_EQUAL(0, SizedString_strcmp(&afterComma, "X"));
}

TEST(SizedString, splitStringFailToFindSeparator)
{
    testSizedStringInit("256.X");
    
    SizedString beforeComma;
    SizedString afterComma;
    SizedString_SplitString(&m_sizedString, ',', &beforeComma, &afterComma);
    LONGS_EQUAL(0, SizedString_strcmp(&beforeComma, "256.X"));
    POINTERS_EQUAL(NULL, afterComma.pString);
    LONGS_EQUAL(0, afterComma.stringLength);
}

TEST(SizedString, strlenOnBlankString)
{
    testSizedStringInit("");
    LONGS_EQUAL(0, SizedString_strlen(&m_sizedString));
}

TEST(SizedString, strlenOnSizedString)
{
    testSizedStringInit("Test string");
    LONGS_EQUAL(11, SizedString_strlen(&m_sizedString));
}

TEST(SizedString, EnumBlankString)
{
    const char* pEnumerator = NULL;
    testSizedStringInit("");
    SizedString_EnumStart(&m_sizedString, &pEnumerator);
    LONGS_EQUAL('\0', SizedString_EnumNext(&m_sizedString, &pEnumerator));
}

TEST(SizedString, EnumSizedString)
{
    static const char testString[] = "Test string";
    const char*       pEnumerator = NULL;
    char              nextChar = '\0';
    
    testSizedStringInit(testString);
    SizedString_EnumStart(&m_sizedString, &pEnumerator);
    for (int i = 0 ; (nextChar = SizedString_EnumNext(&m_sizedString, &pEnumerator)) != '\0' ; i++)
    {
        LONGS_EQUAL(testString[i], nextChar);
    }
}
