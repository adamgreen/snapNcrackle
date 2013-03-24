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
    #include "SizedString.h"
    #include "try_catch.h"
    #include "MallocFailureInject.h"
    #include "util.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(SizedString)
{
    const char* m_pExpectedString;
    const char* m_pExpectedContent;
    char*       m_pDupeString;
    const char* m_pEndPtr;
    size_t      m_expectedStringLength;
    SizedString m_sizedString;
    SizedString m_bufferString;
    int         m_verifyString;
    char        m_testString[64];
    
    void setup()
    {
        clearExceptionCode();
        memset(&m_sizedString, 0xff, sizeof(m_sizedString));
        m_verifyString = FALSE;
        m_pDupeString = NULL;
        m_pEndPtr = NULL;
    }

    void teardown()
    {
        MallocFailureInject_Restore();
        free(m_pDupeString);
        if (m_verifyString)
        {
            POINTERS_EQUAL(m_pExpectedString, m_sizedString.pString);
            LONGS_EQUAL(m_expectedStringLength, m_sizedString.stringLength);
            CHECK_TRUE(0 == strncmp(m_pExpectedContent, m_sizedString.pString, m_sizedString.stringLength));
        }
        LONGS_EQUAL(noException, getExceptionCode());
    }
    
    void testSizedStringInit(const char* pString, size_t stringLength)
    {
        m_sizedString = SizedString_Init(pString, stringLength);
        m_pExpectedString = pString;
        m_pExpectedContent = pString;
        m_expectedStringLength = stringLength;
        m_verifyString = TRUE;
    }
    
    void testSizedStringInit(const char* pString)
    {
        m_sizedString = SizedString_InitFromString(pString);
        m_pExpectedString = pString;
        m_pExpectedContent = pString;
        m_expectedStringLength = pString ? strlen(pString) : 0;
        m_verifyString = TRUE;
    }
    
    SizedString* toSizedString(const char* pString)
    {
        m_bufferString = SizedString_InitFromString(pString);
        return &m_bufferString;
    }
    
    void strcmpValidate(const char* p1, const char* p2, int actualResult)
    {
        LONGS_EQUAL(constrainResult(strcmp(p1, p2)), constrainResult(actualResult));
    }
    
    void strcasecmpValidate(const char* p1, const char* p2, int actualResult)
    {
        LONGS_EQUAL(constrainResult(strcasecmp(p1, p2)), constrainResult(actualResult));
    }
    
    int constrainResult(int result)
    {
        if (result < 0)
            return -1;
        if (result > 0)
            return 1;
        return 0;
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
    CHECK_TRUE(SizedString_IsNull(&m_sizedString));
}

TEST(SizedString, EmptyInit)
{
    testSizedStringInit("");
    CHECK_FALSE(SizedString_IsNull(&m_sizedString));
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

TEST(SizedString, strchrFindsFirstOccurance)
{
    testSizedStringInit("aAa");
    const char* pResult = SizedString_strchr(&m_sizedString, 'a');
    CHECK_TRUE(pResult != NULL);
    POINTERS_EQUAL(m_sizedString.pString, pResult);
    CHECK_TRUE(*pResult == 'a');
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
TEST(SizedString, strrchrInEmptyString)
{
    testSizedStringInit("");
    CHECK_TRUE(NULL == SizedString_strrchr(&m_sizedString, ' '));
}

TEST(SizedString, strrchrOnlyCharInString)
{
    testSizedStringInit("A");
    const char* pResult = SizedString_strrchr(&m_sizedString, 'A');
    CHECK_TRUE(pResult != NULL);
    CHECK_TRUE(*pResult == 'A');
}

TEST(SizedString, strrchrFindsLastOccurance)
{
    testSizedStringInit("aAa");
    const char* pResult = SizedString_strrchr(&m_sizedString, 'a');
    CHECK_TRUE(pResult != NULL);
    POINTERS_EQUAL(m_sizedString.pString + 2, pResult);
    CHECK_TRUE(*pResult == 'a');
}

TEST(SizedString, strrchrCaseSensitive)
{
    testSizedStringInit("A");
    POINTERS_EQUAL(NULL, SizedString_strrchr(&m_sizedString, 'a'));
}

TEST(SizedString, strrchrLastCharInTruncatedString)
{
    testSizedStringInit("123456789", 8);
    const char* pResult = SizedString_strrchr(&m_sizedString, '8');
    CHECK_TRUE(pResult != NULL);
    CHECK_TRUE(*pResult == '8');
}

TEST(SizedString, strrchrPastLastCharInTruncatedString)
{
    testSizedStringInit("123456789", 8);
    POINTERS_EQUAL(NULL, SizedString_strrchr(&m_sizedString, '9'));
}

TEST(SizedString, strcmpMatchWholeString)
{
    testSizedStringInit("Test String");
    strcmpValidate("Test String", "Test String", SizedString_strcmp(&m_sizedString, "Test String"));
}

TEST(SizedString, strcmpMatchWholeTruncatedString)
{
    testSizedStringInit("Test String", 4);
    strcmpValidate("Test", "Test", SizedString_strcmp(&m_sizedString, "Test"));
}

TEST(SizedString, strcmpFailMatchDueToCaseSensitivity)
{
    testSizedStringInit("Test string");
    strcmpValidate("Test string", "Test String", SizedString_strcmp(&m_sizedString, "Test String"));
}

TEST(SizedString, strcmpFailMatchDueToSearchStringBeingTooShort)
{
    testSizedStringInit("Test string");
    strcmpValidate("Test string", "Test", SizedString_strcmp(&m_sizedString, "Test"));
}

TEST(SizedString, strcmpFailMatchDueToSizedStringBeingTooShort)
{
    testSizedStringInit("Test string", 4);
    strcmpValidate("Test", "Test string", SizedString_strcmp(&m_sizedString, "Test string"));
}

TEST(SizedString, strcasecmpMatchWholeStringWithDifferentCase)
{
    testSizedStringInit("Test String");
    strcasecmpValidate("Test String", "tEST sTRING", SizedString_strcasecmp(&m_sizedString, "tEST sTRING"));
}

TEST(SizedString, strcasecmpMatchWholeTruncatedString)
{
    testSizedStringInit("Test String", 4);
    strcasecmpValidate("Test", "tEST", SizedString_strcasecmp(&m_sizedString, "tEST"));
}

TEST(SizedString, strcasecmpFailMatch)
{
    testSizedStringInit("Test string");
    strcasecmpValidate("Test string", "Tst", SizedString_strcasecmp(&m_sizedString, "Tst"));
}

TEST(SizedString, strcasecmpFailMatchDueToSearchStringBeingTooShort)
{
    testSizedStringInit("Test string");
    strcasecmpValidate("Test string", "Test", SizedString_strcasecmp(&m_sizedString, "Test"));
}

TEST(SizedString, strcasecmpFailMatchDueToSizedStringBeingTooShort)
{
    testSizedStringInit("Test string", 4);
    strcasecmpValidate("Test", "Test string", SizedString_strcasecmp(&m_sizedString, "Test string"));
}

TEST(SizedString, CompareMatchWholeString)
{
    testSizedStringInit("Test String");
    strcmpValidate("Test String", "Test String", SizedString_Compare(&m_sizedString, toSizedString("Test String")));
}

TEST(SizedString, CompareMatchWholeTruncatedString)
{
    testSizedStringInit("Test String", 4);
    strcmpValidate("Test", "Test", SizedString_Compare(&m_sizedString, toSizedString("Test")));
}

TEST(SizedString, CompareFailMatchDueToCaseSensitivity)
{
    testSizedStringInit("Test string");
    strcmpValidate("Test string", "Test String", SizedString_Compare(&m_sizedString, toSizedString("Test String")));
}

TEST(SizedString, CompareFailMatchDueToSearchStringBeingTooShort)
{
    testSizedStringInit("Test string");
    strcmpValidate("Test string", "Test", SizedString_Compare(&m_sizedString, toSizedString("Test")));
}

TEST(SizedString, CompareFailMatchDueToSizedStringBeingTooShort)
{
    testSizedStringInit("Test string", 4);
    strcmpValidate("Test", "Test string", SizedString_Compare(&m_sizedString, toSizedString("Test string")));
}

TEST(SizedString, strdupEmptyString)
{
    testSizedStringInit("");
    m_pDupeString = SizedString_strdup(&m_sizedString);
    STRCMP_EQUAL(m_pDupeString, "");
}

TEST(SizedString, strdupShortString)
{
    testSizedStringInit("Test string");
    m_pDupeString = SizedString_strdup(&m_sizedString);
    STRCMP_EQUAL(m_pDupeString, "Test string");
}

TEST(SizedString, strdupTruncatedString)
{
    SizedString testString = SizedString_InitFromString("test 1 2 3");
    testString.stringLength = 4;
    m_pDupeString = SizedString_strdup(&testString);
    STRCMP_EQUAL(m_pDupeString, "test");
}

TEST(SizedString, FailAllocationDuringStrdup)
{
    testSizedStringInit("");
    MallocFailureInject_FailAllocation(1);
        __try_and_catch( m_pDupeString = SizedString_strdup(&m_sizedString) );
    MallocFailureInject_Restore();
    POINTERS_EQUAL(NULL, m_pDupeString);
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
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

TEST(SizedString, strtoulOnNullStringPointerNullEndPtrNoBase)
{
    testSizedStringInit(NULL);
    LONGS_EQUAL(0, SizedString_strtoul(NULL, NULL, 0));
}

TEST(SizedString, strtoulOnNullStringEndPtrNoBase)
{
    const char* m_pEndPtr = NULL;
    testSizedStringInit(NULL);
    LONGS_EQUAL(0, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 0));
    POINTERS_EQUAL(NULL, m_pEndPtr);
}

TEST(SizedString, strtoulOnOneInvalidNegativeBase)
{
    const char* m_pEndPtr = NULL;
    testSizedStringInit("1");
    LONGS_EQUAL(0, SizedString_strtoul(&m_sizedString, &m_pEndPtr, -1));
    POINTERS_EQUAL(m_sizedString.pString, m_pEndPtr);
}

TEST(SizedString, strtoulOnOneInvalid1Base)
{
    const char* m_pEndPtr = NULL;
    testSizedStringInit("1");
    LONGS_EQUAL(0, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 1));
    POINTERS_EQUAL(m_sizedString.pString, m_pEndPtr);
}

TEST(SizedString, strtoulOnInvalid37Base)
{
    const char* m_pEndPtr = NULL;
    testSizedStringInit("1");
    LONGS_EQUAL(0, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 37));
    POINTERS_EQUAL(m_sizedString.pString, m_pEndPtr);
}

TEST(SizedString, strtoulOnBlankStringEndPtrNoBase)
{
    const char* m_pEndPtr = NULL;
    testSizedStringInit("");
    LONGS_EQUAL(0, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 0));
    POINTERS_EQUAL(m_sizedString.pString, m_pEndPtr);
}

TEST(SizedString, strtoulOnZeroNullEndPtrNoBase)
{
    testSizedStringInit("0");
    LONGS_EQUAL(0, SizedString_strtoul(&m_sizedString, NULL, 0));
}

TEST(SizedString, strtoulOnOneNoBase)
{
    testSizedStringInit("1");
    LONGS_EQUAL(1, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 0));
    POINTERS_EQUAL(m_sizedString.pString+1, m_pEndPtr);
}

TEST(SizedString, strtoulOnLargeNumberNoBase)
{
    testSizedStringInit("4294967295");
    LONGS_EQUAL(4294967295U, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 0));
    POINTERS_EQUAL(m_sizedString.pString + 10, m_pEndPtr);
}

TEST(SizedString, strtoulOnNumberDecimalBase)
{
    testSizedStringInit("12345");
    LONGS_EQUAL(12345, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 10));
    POINTERS_EQUAL(m_sizedString.pString + 5, m_pEndPtr);
}

TEST(SizedString, strtoulOnZeroHexBase)
{
    testSizedStringInit("0");
    LONGS_EQUAL(0, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 16));
    POINTERS_EQUAL(m_sizedString.pString + 1, m_pEndPtr);
}

TEST(SizedString, strtoulOnLargeHexBase)
{
    testSizedStringInit("FFFFFFFF");
    LONGS_EQUAL(0xFFFFFFFF, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 16));
    POINTERS_EQUAL(m_sizedString.pString + 8, m_pEndPtr);
}

TEST(SizedString, strtoulBasicOctalBase)
{
    testSizedStringInit("10");
    LONGS_EQUAL(010, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 8));
    POINTERS_EQUAL(m_sizedString.pString + 2, m_pEndPtr);
}

TEST(SizedString, strtoulOnHexWithDefaultBase)
{
    testSizedStringInit("0xf");
    LONGS_EQUAL(0xf, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 0));
    POINTERS_EQUAL(m_sizedString.pString + 3, m_pEndPtr);
}

TEST(SizedString, strtoulOnHexWithHexBase)
{
    testSizedStringInit("0xf");
    LONGS_EQUAL(0xf, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 16));
    POINTERS_EQUAL(m_sizedString.pString + 3, m_pEndPtr);
}

TEST(SizedString, strtoulOnUpperCaseHexPrefix)
{
    testSizedStringInit("0Xf");
    LONGS_EQUAL(0xf, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 0));
    POINTERS_EQUAL(m_sizedString.pString + 3, m_pEndPtr);
}

TEST(SizedString, strtoulOnOctalWithDefaultBase)
{
    testSizedStringInit("010");
    LONGS_EQUAL(010, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 0));
    POINTERS_EQUAL(m_sizedString.pString + 3, m_pEndPtr);
}

TEST(SizedString, strtoulOnOctalWithOctalBase)
{
    testSizedStringInit("010");
    LONGS_EQUAL(010, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 8));
    POINTERS_EQUAL(m_sizedString.pString + 3, m_pEndPtr);
}

TEST(SizedString, strtoulBase36)
{
    testSizedStringInit("aZ");
    LONGS_EQUAL(10*36+35, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 36));
    POINTERS_EQUAL(m_sizedString.pString + 2, m_pEndPtr);
}

TEST(SizedString, strtoulOnDecimalWithNonDigitTerminator)
{
    testSizedStringInit("12*");
    LONGS_EQUAL(12, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 10));
    POINTERS_EQUAL(m_sizedString.pString + 2, m_pEndPtr);
}

TEST(SizedString, strtoulOnDecimalWithCharacterTerminator)
{
    testSizedStringInit("12a");
    LONGS_EQUAL(12, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 10));
    POINTERS_EQUAL(m_sizedString.pString + 2, m_pEndPtr);
}

TEST(SizedString, strtoulOnDecimalOverflow)
{
    testSizedStringInit("4294967296");
    LONGS_EQUAL(4294967295U, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 0));
    POINTERS_EQUAL(m_sizedString.pString + 10, m_pEndPtr);
}

TEST(SizedString, strtoulOnVeryLongDecimalWhichOverflows)
{
    testSizedStringInit("12345678901234567890");
    LONGS_EQUAL(4294967295U, SizedString_strtoul(&m_sizedString, &m_pEndPtr, 0));
    POINTERS_EQUAL(m_sizedString.pString + 20, m_pEndPtr);
}

TEST(SizedString, EnumBlankString)
{
    const char* pEnumerator = NULL;
    testSizedStringInit("");
    SizedString_EnumStart(&m_sizedString, &pEnumerator);
    LONGS_EQUAL('\0', SizedString_EnumCurr(&m_sizedString, pEnumerator));
    LONGS_EQUAL('\0', SizedString_EnumNext(&m_sizedString, &pEnumerator));
    LONGS_EQUAL(0 , SizedString_EnumRemaining(&m_sizedString, pEnumerator));
}

TEST(SizedString, EnumRemainingOnInvalidEnumerator)
{
    const char* pEnumerator = NULL;
    testSizedStringInit("");
    LONGS_EQUAL(0 , SizedString_EnumRemaining(&m_sizedString, pEnumerator));
}

TEST(SizedString, EnumSizedString)
{
    static const char testString[] = "Test string";
    const char*       pEnumerator = NULL;
    size_t            remaining = strlen(testString);
    
    testSizedStringInit(testString);
    SizedString_EnumStart(&m_sizedString, &pEnumerator);
    for (int i = 0 ; ; i++)
    {
        LONGS_EQUAL(remaining--, SizedString_EnumRemaining(&m_sizedString, pEnumerator));
        LONGS_EQUAL(testString[i], SizedString_EnumCurr(&m_sizedString, pEnumerator));
        char nextChar = SizedString_EnumNext(&m_sizedString, &pEnumerator);
        LONGS_EQUAL(testString[i], nextChar);
        if ('\0' == nextChar)
            break;
    }
}
