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
    #include "TextFileSource.h"
    #include "MallocFailureInject.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"

TEST_GROUP(TextFileSource)
{
    TextSource* m_pTextSource;
    
    void setup()
    {
        m_pTextSource = NULL;
        clearExceptionCode();
    }

    void teardown()
    {
        MallocFailureInject_Restore();
        TextSource_FreeAll();
        LONGS_EQUAL(noException, getExceptionCode());
    }
    
    void validateOutOfMemoryExceptionThrown()
    {
        LONGS_EQUAL(outOfMemoryException, getExceptionCode());
        clearExceptionCode();
        POINTERS_EQUAL(NULL, m_pTextSource);
    }

    void createTestSource(const char* pTestText)
    {
        m_pTextSource = createTextFileSource(pTestText);
    }
    
    TextSource* createTextFileSource(const char* pTestText)
    {
        TextFile* pTextFile = TextFile_CreateFromString(pTestText);
        return TextFileSource_Create(pTextFile);
    }
};


TEST(TextFileSource, Create)
{
    createTestSource(" \n \n");
    CHECK(m_pTextSource);
}

TEST(TextFileSource, FailAllocationToCreate)
{
    static const int allocationsToFail = 1;
    TextFile* pTextFile = TextFile_CreateFromString(" \n \n");
    for (int i = 1 ; i <= allocationsToFail ; i++)
    {
        MallocFailureInject_FailAllocation(i);
            __try_and_catch( m_pTextSource = TextFileSource_Create(pTextFile) );
            validateOutOfMemoryExceptionThrown();
            CHECK(NULL == m_pTextSource);
        MallocFailureInject_Restore();
    }

    MallocFailureInject_FailAllocation(allocationsToFail + 1);
        m_pTextSource = TextFileSource_Create(pTextFile);
        CHECK(m_pTextSource);
}

TEST(TextFileSource, IsEndOfFileReturnFalse)
{
    createTestSource(" \n");
    CHECK_FALSE(TextSource_IsEndOfFile(m_pTextSource));
}

TEST(TextFileSource, IsEndOfFileReturnTrue)
{
    createTestSource("");
    CHECK_TRUE(TextSource_IsEndOfFile(m_pTextSource));
}

TEST(TextFileSource, GetNextLine)
{
    createTestSource(" \n");
    SizedString nextLine = TextSource_GetNextLine(m_pTextSource);
    CHECK(0 == SizedString_strcmp(&nextLine, " "));
    CHECK_TRUE(TextSource_IsEndOfFile(m_pTextSource));
}

TEST(TextFileSource, GetLineNumber)
{
    createTestSource(" \n");
    LONGS_EQUAL(0, TextSource_GetLineNumber(m_pTextSource));
    TextSource_GetNextLine(m_pTextSource);
    LONGS_EQUAL(1, TextSource_GetLineNumber(m_pTextSource));
}

TEST(TextFileSource, GetFilename)
{
    createTestSource(" \n \n");
    STRCMP_EQUAL("filename", TextSource_GetFilename(m_pTextSource));
}

TEST(TextFileSource, StackPush)
{
    TextSource* pStack = NULL;
    createTestSource(" \n");
    TextSource_StackPush(&pStack, m_pTextSource);
    POINTERS_EQUAL(m_pTextSource, pStack);
}

TEST(TextFileSource, StackPushAndPop2Items)
{
    TextSource* pTextSource1 = createTextFileSource(" \n");
    TextSource* pTextSource2 = createTextFileSource(" \n");
    
    TextSource* pStack = NULL;
    LONGS_EQUAL(0, TextSource_StackDepth(pStack));
    TextSource_StackPush(&pStack, pTextSource1);
    LONGS_EQUAL(1, TextSource_StackDepth(pStack));
    TextSource_StackPush(&pStack, pTextSource2);
    LONGS_EQUAL(2, TextSource_StackDepth(pStack));

    POINTERS_EQUAL(pTextSource2, pStack);
    TextSource_StackPop(&pStack);
    POINTERS_EQUAL(pTextSource1, pStack);
    LONGS_EQUAL(1, TextSource_StackDepth(pStack));
    TextSource_StackPop(&pStack);
    POINTERS_EQUAL(NULL, pStack);
    LONGS_EQUAL(0, TextSource_StackDepth(pStack));
}

TEST(TextFileSource, StackPopWhenEmpty)
{
    TextSource* pStack = NULL;
    TextSource_StackPop(&pStack);
}

TEST(TextFileSource, GetTextFile)
{
    TextFile* pTextFile = TextFile_CreateFromString(" \n");
    TextSource* pTextSource = TextFileSource_Create(pTextFile);
    POINTERS_EQUAL(pTextFile, TextSource_GetTextFile(pTextSource));
}
