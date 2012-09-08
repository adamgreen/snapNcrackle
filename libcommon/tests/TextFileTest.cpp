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
    #include "FileFailureInject.h"
    #include "util.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"

static const char* tempFilename = "TestFileTestCppBogusContent.S";

TEST_GROUP(TextFile)
{
    TextFile*   m_pTextFile;
    SizedString m_tempFilename;
    char        m_buffer[128];
    
    void setup()
    {
        clearExceptionCode();
        m_pTextFile = NULL;
        m_buffer[0] = '\0';
        m_tempFilename = SizedString_InitFromString(tempFilename);
    }

    void teardown()
    {
        MallocFailureInject_Restore();
        TextFile_Free(m_pTextFile);
        LONGS_EQUAL(noException, getExceptionCode());
        remove(tempFilename);
    }
    
    void validateEndOfFileForNextLine()
    {
        const char* pLine = TextFile_GetNextLine(m_pTextFile);
        POINTERS_EQUAL(NULL, pLine);
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
    
    void createTestFile(const char* pTestText)
    {
        FILE* pFile = fopen(tempFilename, "w");
        CHECK(pFile != NULL);
        LONGS_EQUAL(strlen(pTestText), fwrite(pTestText, 1, strlen(pTestText), pFile));
        fclose(pFile);
    }
    
    void validateExceptionThrown(int expectedExceptionCode)
    {
        POINTERS_EQUAL(NULL, m_pTextFile);
        LONGS_EQUAL(expectedExceptionCode, getExceptionCode());
        clearExceptionCode();
    }
};


TEST(TextFile, FailAllInitAllocation)
{
    static const int allocationsToFail = 1;

    for (int i = 1 ; i <= allocationsToFail ; i++)
    {
        MallocFailureInject_FailAllocation(i);
            __try_and_catch( m_pTextFile = TextFile_CreateFromString(dupe("")) );
        POINTERS_EQUAL(NULL, m_pTextFile);
        validateExceptionThrown(outOfMemoryException);
    }

    MallocFailureInject_FailAllocation(allocationsToFail + 1);
    m_pTextFile = m_pTextFile = TextFile_CreateFromString(dupe(""));
    CHECK_TRUE(m_pTextFile != NULL);
}

TEST(TextFile, GetLineFromEmptyString)
{
    m_pTextFile = TextFile_CreateFromString(dupe(""));
    CHECK(NULL != m_pTextFile);
    validateEndOfFileForNextLine();
}

TEST(TextFile, GetLineFromTextWithNoLineTerminators)
{
    m_pTextFile = TextFile_CreateFromString(dupe(" "));
    CHECK(NULL != m_pTextFile);
    
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, GetLineFromSingleLineWithNewline)
{
    m_pTextFile = TextFile_CreateFromString(dupe(" \n"));
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, GetLineFromSingleLineWithCarriageReturn)
{
    m_pTextFile = TextFile_CreateFromString(dupe(" \r"));
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, GetLineFromSingleLineWithCarriageReturnAndLineFeed)
{
    m_pTextFile = TextFile_CreateFromString(dupe(" \r\n"));
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, GetLineFromSingleLineWithLineFeedAndCarriageReturn)
{
    m_pTextFile = TextFile_CreateFromString(dupe(" \n\r"));
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, GetLine2LinesWithCarriageReturnAndNewLine)
{
    m_pTextFile = TextFile_CreateFromString(dupe(" \n\r \n\r"));
    fetchAndValidateLineWithSingleSpace();
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, GetLine2LinesWithCarriageReturnAndNewLineButEOFAtEndOfSecondLine)
{
    m_pTextFile = TextFile_CreateFromString(dupe(" \n\r "));
    fetchAndValidateLineWithSingleSpace();
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, CreateFromFile)
{
    createTestFile(" \n\r \n");
    m_pTextFile = TextFile_CreateFromFile(&m_tempFilename);
    fetchAndValidateLineWithSingleSpace();
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, FailAllCreateFromFileAllocations)
{
    static const int allocationsToFail = 3;
    createTestFile("\n\r");

    for (int i = 1 ; i <= allocationsToFail ; i++)
    {
        MallocFailureInject_FailAllocation(i);
            __try_and_catch( m_pTextFile = TextFile_CreateFromFile(&m_tempFilename) );
        POINTERS_EQUAL(NULL, m_pTextFile);
        validateExceptionThrown(outOfMemoryException);
    }

    MallocFailureInject_FailAllocation(allocationsToFail + 1);
    m_pTextFile = TextFile_CreateFromFile(&m_tempFilename);
    CHECK_TRUE(m_pTextFile != NULL);
}

TEST(TextFile, FailFOpen)
{
    createTestFile("\n\r");
    fopenFail(NULL);
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(&m_tempFilename) );
    fopenRestore();

    POINTERS_EQUAL(NULL, m_pTextFile);
    LONGS_EQUAL(fileNotFoundException, getExceptionCode());
    clearExceptionCode();
}

TEST(TextFile, FailFSeekToEOF)
{
    createTestFile("\n\r");
    fseekSetFailureCode(-1);
    fseekSetCallsBeforeFailure(0);
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(&m_tempFilename) );
    fseekRestore();
    validateExceptionThrown(fileException);
}

TEST(TextFile, FailFSeekBackToStart)
{
    createTestFile("\n\r");
    fseekSetFailureCode(-1);
    fseekSetCallsBeforeFailure(1);
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(&m_tempFilename) );
    fseekRestore();
    validateExceptionThrown(fileException);
}

TEST(TextFile, FailFTell)
{
    createTestFile("\n\r");
    ftellFail(-1);
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(&m_tempFilename) );
    ftellRestore();
    validateExceptionThrown(fileException);
}

TEST(TextFile, FailFRead)
{
    createTestFile("\n\r");
    freadFail(0);
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(&m_tempFilename) );
    freadRestore();
    validateExceptionThrown(fileException);
}

TEST(TextFile, GetLineNumberBeforeFirstGetLineShouldReturn0)
{
    m_pTextFile = TextFile_CreateFromString(dupe(""));
    LONGS_EQUAL(0, TextFile_GetLineNumber(m_pTextFile));
}

TEST(TextFile, GetLineNumberForFirstLine)
{
    m_pTextFile = TextFile_CreateFromString(dupe(" \n"));
    fetchAndValidateLineWithSingleSpace();
    LONGS_EQUAL(1, TextFile_GetLineNumber(m_pTextFile));
    validateEndOfFileForNextLine();
    LONGS_EQUAL(1, TextFile_GetLineNumber(m_pTextFile));
}

TEST(TextFile, GetLineNumberForFirstTwoLines)
{
    m_pTextFile = TextFile_CreateFromString(dupe(" \n"
                                                 " "));
    fetchAndValidateLineWithSingleSpace();
    LONGS_EQUAL(1, TextFile_GetLineNumber(m_pTextFile));
    fetchAndValidateLineWithSingleSpace();
    LONGS_EQUAL(2, TextFile_GetLineNumber(m_pTextFile));
    validateEndOfFileForNextLine();
    LONGS_EQUAL(2, TextFile_GetLineNumber(m_pTextFile));
}

TEST(TextFile, GetFilenameOnStringBasedTextFileShouldBeFilename)
{
    m_pTextFile = TextFile_CreateFromString(dupe(""));
    LONGS_EQUAL(0, SizedString_strcmp(TextFile_GetFilename(m_pTextFile), "filename"));
}

TEST(TextFile, GetFilenameOnFileBasedTextFile)
{
    createTestFile(" \n\r \n");
    m_pTextFile = TextFile_CreateFromFile(&m_tempFilename);
    LONGS_EQUAL(0, SizedString_Compare(TextFile_GetFilename(m_pTextFile), &m_tempFilename));
}
