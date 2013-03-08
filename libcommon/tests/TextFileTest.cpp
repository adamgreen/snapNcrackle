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
    TextFile*   m_pTextFileDerived;
    SizedString m_sizedString;
    char        m_buffer[128];
    
    void setup()
    {
        clearExceptionCode();
        m_pTextFile = NULL;
        m_pTextFileDerived = NULL;
        m_buffer[0] = '\0';
    }

    void teardown()
    {
        MallocFailureInject_Restore();
        TextFile_Free(m_pTextFileDerived);
        TextFile_Free(m_pTextFile);
        LONGS_EQUAL(noException, getExceptionCode());
        remove(tempFilename);
    }
    
    void validateEndOfFileForNextLine()
    {
        validateEndOfFileForNextLine(m_pTextFile);
    }
    
    void validateEndOfFileForNextLine(TextFile* pTextFile)
    {
        CHECK_TRUE(TextFile_IsEndOfFile(pTextFile));
        SizedString line = TextFile_GetNextLine(pTextFile);
        CHECK_TRUE(SizedString_IsNull(&line));
    }
    
    SizedString* toSizedString(const char* pString)
    {
        m_sizedString = SizedString_InitFromString(pString);
        return &m_sizedString;
    }
    
    void fetchAndValidateLineWithSingleSpace()
    {
        fetchAndValidateLineWithSingleSpace(m_pTextFile);
    }

    void fetchAndValidateLineWithSingleSpace(TextFile* pTextFile)
    {
        CHECK_FALSE(TextFile_IsEndOfFile(pTextFile));
        SizedString line = TextFile_GetNextLine(pTextFile);
        CHECK_TRUE(0 == SizedString_strcmp(&line, " "));
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
    static const int allocationsToFail = 2;

    for (int i = 1 ; i <= allocationsToFail ; i++)
    {
        MallocFailureInject_FailAllocation(i);
            __try_and_catch( m_pTextFile = TextFile_CreateFromString("") );
        POINTERS_EQUAL(NULL, m_pTextFile);
        validateExceptionThrown(outOfMemoryException);
    }

    MallocFailureInject_FailAllocation(allocationsToFail + 1);
    m_pTextFile = m_pTextFile = TextFile_CreateFromString("");
    CHECK_TRUE(m_pTextFile != NULL);
}

TEST(TextFile, GetLineFromEmptyString)
{
    m_pTextFile = TextFile_CreateFromString("");
    CHECK(NULL != m_pTextFile);
    validateEndOfFileForNextLine();
}

TEST(TextFile, GetLineFromTextWithNoLineTerminators)
{
    m_pTextFile = TextFile_CreateFromString(" ");
    CHECK(NULL != m_pTextFile);
    
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, GetLineFromSingleLineWithNewline)
{
    m_pTextFile = TextFile_CreateFromString(" \n");
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, GetLineFromSingleLineWithCarriageReturn)
{
    m_pTextFile = TextFile_CreateFromString(" \r");
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, GetLineFromSingleLineWithCarriageReturnAndLineFeed)
{
    m_pTextFile = TextFile_CreateFromString(" \r\n");
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, GetLineFromSingleLineWithLineFeedAndCarriageReturn)
{
    m_pTextFile = TextFile_CreateFromString(" \n\r");
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, GetLine2LinesWithCarriageReturnAndNewLine)
{
    m_pTextFile = TextFile_CreateFromString(" \n\r \n\r");
    fetchAndValidateLineWithSingleSpace();
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, GetLine2LinesWithCarriageReturnAndNewLineButEOFAtEndOfSecondLine)
{
    m_pTextFile = TextFile_CreateFromString(" \n\r ");
    fetchAndValidateLineWithSingleSpace();
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}


TEST(TextFile, CreateFromFile)
{
    createTestFile(" \n\r \n");
    m_pTextFile = TextFile_CreateFromFile(NULL, toSizedString(tempFilename), NULL);
    fetchAndValidateLineWithSingleSpace();
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, CreateFromFileWithSuffix)
{
    createTestFile(" \n\r \n");
    m_pTextFile = TextFile_CreateFromFile(NULL, toSizedString("TestFileTestCppBogusContent"), ".S");
    fetchAndValidateLineWithSingleSpace();
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, CreateFromFileWithSuffixAndDirectoryNoSlash)
{
    createTestFile(" \n\r \n");
    SizedString testDirectory = SizedString_InitFromString(".");
    m_pTextFile = TextFile_CreateFromFile(&testDirectory, toSizedString("TestFileTestCppBogusContent"), ".S");
    fetchAndValidateLineWithSingleSpace();
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, CreateFromFileWithSuffixAndDirectorySlash)
{
    createTestFile(" \n\r \n");
    SizedString testDirectory = SizedString_InitFromString("./");
    m_pTextFile = TextFile_CreateFromFile(&testDirectory, toSizedString("TestFileTestCppBogusContent"), ".S");
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
            __try_and_catch( m_pTextFile = TextFile_CreateFromFile(NULL, toSizedString(tempFilename), NULL) );
        POINTERS_EQUAL(NULL, m_pTextFile);
        validateExceptionThrown(outOfMemoryException);
    }

    MallocFailureInject_FailAllocation(allocationsToFail + 1);
    m_pTextFile = TextFile_CreateFromFile(NULL, toSizedString(tempFilename), NULL);
    CHECK_TRUE(m_pTextFile != NULL);
}

TEST(TextFile, FailWithBadDirectory)
{
    createTestFile("\n\r");
    SizedString testDirectory = SizedString_InitFromString("foo.bar");
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(&testDirectory, toSizedString(tempFilename), NULL) );

    POINTERS_EQUAL(NULL, m_pTextFile);
    LONGS_EQUAL(fileNotFoundException, getExceptionCode());
    clearExceptionCode();
}

TEST(TextFile, FailFOpen)
{
    createTestFile("\n\r");
    fopenFail(NULL);
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(NULL, toSizedString(tempFilename), NULL) );
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
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(NULL, toSizedString(tempFilename), NULL) );
    fseekRestore();
    validateExceptionThrown(fileException);
}

TEST(TextFile, FailFSeekBackToStart)
{
    createTestFile("\n\r");
    fseekSetFailureCode(-1);
    fseekSetCallsBeforeFailure(1);
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(NULL, toSizedString(tempFilename), NULL) );
    fseekRestore();
    validateExceptionThrown(fileException);
}

TEST(TextFile, FailFTell)
{
    createTestFile("\n\r");
    ftellFail(-1);
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(NULL, toSizedString(tempFilename), NULL) );
    ftellRestore();
    validateExceptionThrown(fileException);
}

TEST(TextFile, FailFRead)
{
    createTestFile("\n\r");
    freadFail(0);
    __try_and_catch( m_pTextFile = TextFile_CreateFromFile(NULL, toSizedString(tempFilename), NULL) );
    freadRestore();
    validateExceptionThrown(fileException);
}

TEST(TextFile, FailAllCreateFromTextFileAllocations)
{
    static const int allocationsToFail = 1;
    m_pTextFile = TextFile_CreateFromString("\n\r");

    for (int i = 1 ; i <= allocationsToFail ; i++)
    {
        MallocFailureInject_FailAllocation(i);
            __try_and_catch( m_pTextFileDerived = TextFile_CreateFromTextFile(m_pTextFile) );
        POINTERS_EQUAL(NULL, m_pTextFileDerived);
        LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    }
    clearExceptionCode();

    MallocFailureInject_FailAllocation(allocationsToFail + 1);
    m_pTextFileDerived = TextFile_CreateFromTextFile(m_pTextFile);
    CHECK_TRUE(m_pTextFileDerived != NULL);
}

TEST(TextFile, CreateFromTextFile)
{
    m_pTextFile = TextFile_CreateFromString(" \n");
    m_pTextFileDerived = TextFile_CreateFromTextFile(m_pTextFile);
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
    fetchAndValidateLineWithSingleSpace(m_pTextFileDerived);
    validateEndOfFileForNextLine(m_pTextFileDerived);
}

TEST(TextFile, CreateFrom2LineTextFileAndResetDerived)
{
    m_pTextFile = TextFile_CreateFromString(" \n \n");
    fetchAndValidateLineWithSingleSpace();
    m_pTextFileDerived = TextFile_CreateFromTextFile(m_pTextFile);
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();

    TextFile_Reset(m_pTextFileDerived);
    fetchAndValidateLineWithSingleSpace(m_pTextFileDerived);
    fetchAndValidateLineWithSingleSpace(m_pTextFileDerived);
    validateEndOfFileForNextLine(m_pTextFileDerived);
}

TEST(TextFile, GetLineNumberBeforeFirstGetLineShouldReturn0)
{
    m_pTextFile = TextFile_CreateFromString("");
    LONGS_EQUAL(0, TextFile_GetLineNumber(m_pTextFile));
}

TEST(TextFile, GetLineNumberForFirstLine)
{
    m_pTextFile = TextFile_CreateFromString(" \n");
    fetchAndValidateLineWithSingleSpace();
    LONGS_EQUAL(1, TextFile_GetLineNumber(m_pTextFile));
    validateEndOfFileForNextLine();
    LONGS_EQUAL(1, TextFile_GetLineNumber(m_pTextFile));
}

TEST(TextFile, GetLineNumberForFirstTwoLines)
{
    m_pTextFile = TextFile_CreateFromString(" \n"
                                            " ");
    fetchAndValidateLineWithSingleSpace();
    LONGS_EQUAL(1, TextFile_GetLineNumber(m_pTextFile));
    fetchAndValidateLineWithSingleSpace();
    LONGS_EQUAL(2, TextFile_GetLineNumber(m_pTextFile));
    validateEndOfFileForNextLine();
    LONGS_EQUAL(2, TextFile_GetLineNumber(m_pTextFile));
}

TEST(TextFile, GetFilenameOnStringBasedTextFileShouldBeFilename)
{
    m_pTextFile = TextFile_CreateFromString("");
    STRCMP_EQUAL(TextFile_GetFilename(m_pTextFile), "filename");
}

TEST(TextFile, GetFilenameOnFileBasedTextFile)
{
    createTestFile(" \n\r \n");
    m_pTextFile = TextFile_CreateFromFile(NULL, toSizedString(tempFilename), NULL);
    STRCMP_EQUAL(TextFile_GetFilename(m_pTextFile), tempFilename);
}

TEST(TextFile, ResetOnSingleLineWithNewline)
{
    m_pTextFile = TextFile_CreateFromString(" \n");
    fetchAndValidateLineWithSingleSpace();
    LONGS_EQUAL(1, TextFile_GetLineNumber(m_pTextFile));
    validateEndOfFileForNextLine();

    TextFile_Reset(m_pTextFile);
    LONGS_EQUAL(0, TextFile_GetLineNumber(m_pTextFile));
    fetchAndValidateLineWithSingleSpace();
    LONGS_EQUAL(1, TextFile_GetLineNumber(m_pTextFile));
    validateEndOfFileForNextLine();
}

TEST(TextFile, SetEndOfFileToEmptyFileBeforeEnumeratingAnyLines)
{
    m_pTextFile = TextFile_CreateFromString(" \n \n");
    TextFile_SetEndOfFile(m_pTextFile);
    validateEndOfFileForNextLine();
}

TEST(TextFile, SetEndOfFileAfterEnumeratingToSecondLineShouldResultInSingleLineFile)
{
    m_pTextFile = TextFile_CreateFromString(" \n \n");
    fetchAndValidateLineWithSingleSpace();
    fetchAndValidateLineWithSingleSpace();
    TextFile_SetEndOfFile(m_pTextFile);

    TextFile_Reset(m_pTextFile);
    fetchAndValidateLineWithSingleSpace();
    validateEndOfFileForNextLine();
}

TEST(TextFile, AdvanceBaseFileToMatchDerivedFile)
{
    m_pTextFile = TextFile_CreateFromString(" \n \n");
    m_pTextFileDerived = TextFile_CreateFromTextFile(m_pTextFile);
    fetchAndValidateLineWithSingleSpace(m_pTextFileDerived);
    LONGS_EQUAL(1, TextFile_GetLineNumber(m_pTextFileDerived));
    TextFile_AdvanceTo(m_pTextFile, m_pTextFileDerived);

    // Validate that base file only sees last line now.
    fetchAndValidateLineWithSingleSpace(m_pTextFile);
    LONGS_EQUAL(2, TextFile_GetLineNumber(m_pTextFile));
    validateEndOfFileForNextLine(m_pTextFile);

    // Validate that derived file also only sees last line.
    fetchAndValidateLineWithSingleSpace(m_pTextFileDerived);
    LONGS_EQUAL(2, TextFile_GetLineNumber(m_pTextFileDerived));
    validateEndOfFileForNextLine(m_pTextFileDerived);
}

TEST(TextFile, AdvanceBaseFileToNonDerivedFileShouldThrow)
{
    m_pTextFile = TextFile_CreateFromString(" \n \n");
    m_pTextFileDerived = TextFile_CreateFromString(" \n");
    __try_and_catch( TextFile_AdvanceTo(m_pTextFile, m_pTextFileDerived) );
    LONGS_EQUAL(invalidArgumentException, getExceptionCode());
    clearExceptionCode();
}
