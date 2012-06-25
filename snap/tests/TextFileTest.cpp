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
    #include "TextFile.h"
    #include "MallocFailureInject.h"
    #include "util.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"

TEST_GROUP(TextFile)
{
    TextFile* m_pTextFile;
    char      m_buffer[128];
    
    void setup()
    {
        clearExceptionCode();
        m_pTextFile = NULL;
        m_buffer[0] = '\0';
    }

    void teardown()
    {
        TextFile_Free(m_pTextFile);
        LONGS_EQUAL(noException, getExceptionCode());
    }
    
    void validateEndOfFileExceptionForNextLine()
    {
        const char* pLine = TextFile_GetNextLine(m_pTextFile);
        LONGS_EQUAL(endOfFileException, getExceptionCode());
        POINTERS_EQUAL(NULL, pLine);
        clearExceptionCode();
    }
    
    char* dupe(const char* pString)
    {
        strcpy(m_buffer, pString);
        return m_buffer;
    }
    
    void fetchAndValidateLineWithSingleSpace()
    {
        const char* pLine = NULL;
    
        pLine = TextFile_GetNextLine(m_pTextFile);
        STRCMP_EQUAL(" ", pLine);
    }
};


TEST(TextFile, FailInitAllocation)
{
    int exceptionThrown = FALSE;
    
    MallocFailureInject_Construct(1);
    __try
        m_pTextFile = TextFile_CreateFromString(dupe(""));
    __catch
        exceptionThrown = TRUE;
    MallocFailureInject_Destruct();
    
    CHECK_TRUE(exceptionThrown);
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    POINTERS_EQUAL(NULL, m_pTextFile);
    clearExceptionCode();
}

TEST(TextFile, GetLineFromEmptyString)
{
    m_pTextFile = TextFile_CreateFromString(dupe(""));
    CHECK(NULL != m_pTextFile);
    validateEndOfFileExceptionForNextLine();
}

TEST(TextFile, GetLineFromTextWithNoLineTerminators)
{
    m_pTextFile = TextFile_CreateFromString(dupe(" "));
    CHECK(NULL != m_pTextFile);
    
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileExceptionForNextLine();
}

TEST(TextFile, GetLineFromSingleLineWithNewline)
{
    m_pTextFile = TextFile_CreateFromString(dupe(" \n"));
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileExceptionForNextLine();
}

TEST(TextFile, GetLineFromSingleLineWithCarriageReturn)
{
    m_pTextFile = TextFile_CreateFromString(dupe(" \r"));
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileExceptionForNextLine();
}

TEST(TextFile, GetLineFromSingleLineWithCarriageReturnAndLineFeed)
{
    m_pTextFile = TextFile_CreateFromString(dupe(" \r\n"));
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileExceptionForNextLine();
}

TEST(TextFile, GetLineFromSingleLineWithLineFeedAndCarriageReturn)
{
    m_pTextFile = TextFile_CreateFromString(dupe(" \n\r"));
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileExceptionForNextLine();
}

TEST(TextFile, GetLine2LinesWithCarriageReturnAndNewLine)
{
    m_pTextFile = TextFile_CreateFromString(dupe(" \n\r \n\r"));
    fetchAndValidateLineWithSingleSpace();
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileExceptionForNextLine();
}

TEST(TextFile, GetLine2LinesWithCarriageReturnAndNewLineButEOFAtEndOfSecondLine)
{
    m_pTextFile = TextFile_CreateFromString(dupe(" \n\r "));
    fetchAndValidateLineWithSingleSpace();
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileExceptionForNextLine();
}