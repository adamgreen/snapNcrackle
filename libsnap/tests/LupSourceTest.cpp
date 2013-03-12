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
    #include "LupSource.h"
    #include "MallocFailureInject.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(LupSource)
{
    void setup()
    {
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
    }
    
    void iterateLinesAndVerifyCount(TextSource* pTextSource, long expectedLineCount)
    {
        long         iterations = 0;
        unsigned int prevLineNumber = 0;
        
        while (!TextSource_IsEndOfFile(pTextSource))
        {
            SizedString nextLine = TextSource_GetNextLine(pTextSource);
            CHECK_TRUE(0 == SizedString_strcmp(&nextLine, " "));
            unsigned int lineNumber = TextSource_GetLineNumber(pTextSource);
            CHECK_TRUE(lineNumber == prevLineNumber + 1 || lineNumber == 1);
            prevLineNumber = lineNumber;
            iterations++;
        }
        LONGS_EQUAL(expectedLineCount, iterations);
    }
};


TEST(LupSource, FailAllocationsToCreate)
{
    TextSource* pTextSource = NULL;
    TextFile*   pTextFile = TextFile_CreateFromString(" \n");
    static const int allocationsToFail = 1;
    for (int i = 1 ; i <= allocationsToFail ; i++)
    {
        MallocFailureInject_FailAllocation(i);
        __try_and_catch( pTextSource = LupSource_Create(pTextFile, 2) );
        validateOutOfMemoryExceptionThrown();
        POINTERS_EQUAL(NULL, pTextSource);
    }

    MallocFailureInject_FailAllocation(allocationsToFail + 1);
    pTextSource = LupSource_Create(pTextFile, 2);
    CHECK(pTextSource);
}

TEST(LupSource, Verify1IterationsOf1Line)
{
    TextFile*   pTextFile = TextFile_CreateFromString(" \n");
    TextSource* pTextSource = LupSource_Create(pTextFile, 1);
    iterateLinesAndVerifyCount(pTextSource, 1);
}

TEST(LupSource, Verify2IterationsOf1Line)
{
    TextFile*   pTextFile = TextFile_CreateFromString(" \n");
    TextSource* pTextSource = LupSource_Create(pTextFile, 2);
    iterateLinesAndVerifyCount(pTextSource, 2);
}

TEST(LupSource, Verify2IterationsOf2Lines)
{
    TextFile*   pTextFile = TextFile_CreateFromString(" \n \n");
    TextSource* pTextSource = LupSource_Create(pTextFile, 2);
    iterateLinesAndVerifyCount(pTextSource, 4);
}

TEST(LupSource, Filename)
{
    TextFile*   pTextFile = TextFile_CreateFromString(" \n \n");
    TextSource* pTextSource = LupSource_Create(pTextFile, 2);
    STRCMP_EQUAL("filename", TextSource_GetFilename(pTextSource));
}

TEST(LupSource, GetTextFile)
{
    TextFile* pTextFile = TextFile_CreateFromString(" \n");
    TextSource* pTextSource = LupSource_Create(pTextFile, 2);
    POINTERS_EQUAL(pTextFile, TextSource_GetTextFile(pTextSource));
}
