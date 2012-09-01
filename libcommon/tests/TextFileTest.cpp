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
    
    void validateFileExceptionThrown()
    {
        POINTERS_EQUAL(NULL, m_pTextFile);
        LONGS_EQUAL(fileException, getExceptionCode());
        clearExceptionCode();
    }
};


TEST(TextFile, FailInitAllocation)
{
    int exceptionThrown = FALSE;
    
    MallocFailureInject_FailAllocation(1);
    __try
        m_pTextFile = TextFile_CreateFromString(dupe(""));
    __catch
        exceptionThrown = TRUE;
    MallocFailureInject_Restore();
    
    CHECK_TRUE(exceptionThrown);
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    POINTERS_EQUAL(NULL, m_pTextFile);
    clearExceptionCode();
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
    m_pTextFile = TextFile_CreateFromFile(tempFilename);
    fetchAndValidateLineWithSingleSpace();
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, FailTextBufferAllocation)
{
    createTestFile("\n\r");
    MallocFailureInject_FailAllocation(1);
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(tempFilename) );
    MallocFailureInject_Restore();
    
    POINTERS_EQUAL(NULL, m_pTextFile);
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}

TEST(TextFile, FailFOpen)
{
    createTestFile("\n\r");
    fopenFail(NULL);
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(tempFilename) );
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
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(tempFilename) );
    fseekRestore();
    validateFileExceptionThrown();
}

TEST(TextFile, FailFSeekBackToStart)
{
    createTestFile("\n\r");
    fseekSetFailureCode(-1);
    fseekSetCallsBeforeFailure(1);
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(tempFilename) );
    fseekRestore();
    validateFileExceptionThrown();
}

TEST(TextFile, FailFTell)
{
    createTestFile("\n\r");
    ftellFail(-1);
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(tempFilename) );
    ftellRestore();
    validateFileExceptionThrown();
}

TEST(TextFile, FailFRead)
{
    createTestFile("\n\r");
    freadFail(0);
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(tempFilename) );
    freadRestore();
    validateFileExceptionThrown();
}
