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
#include "string.h"

// Include headers from C modules under test.
extern "C"
{
#include "CommandLine.h"
#include "CommandLineTest.h"
#include "util.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


static const char g_usageString[] = "Usage:";


TEST_GROUP(SnapCommandLine)
{
    const char*     m_argv[10];
    SnapCommandLine m_commandLine;
    int             m_argc;
    
    void setup()
    {
        clearExceptionCode();

        memset(m_argv, 0, sizeof(m_argv));
        memset(&m_commandLine, 0xff, sizeof(m_commandLine));
        m_argc = 0;

        printfSpy_Hook(strlen(g_usageString));
    }

    void teardown()
    {
        LONGS_EQUAL(noException, getExceptionCode());
        printfSpy_Unhook();
    }

    void addArg(const char* pArg)
    {
        CHECK(m_argc < (int)ARRAYSIZE(m_argv));
        m_argv[m_argc++] = pArg;
    }
    
    void validateSourceFilesAndNoErrorMessage(const char** pFirstSourceFileArg, int expectedSourceFileCount)
    {
        validateNoErrorMessageWasDisplayed();
        validateSourceFiles(pFirstSourceFileArg, expectedSourceFileCount);
    }
    
    void validateNoErrorMessageWasDisplayed(void)
    {
        STRCMP_EQUAL("", printfSpy_GetLastOutput());
    }
    
    void validateSourceFiles(const char** pFirstSourceFileArg, int expectedSourceFileCount)
    {
        LONGS_EQUAL(expectedSourceFileCount, m_commandLine.sourceFileCount);
        CHECK(m_commandLine.pSourceFiles != NULL);
        for (int i = 0 ; i < expectedSourceFileCount ; i++)
            POINTERS_EQUAL(pFirstSourceFileArg[i], m_commandLine.pSourceFiles[i]);
    }
};


TEST(SnapCommandLine, NoParameters)
{
    int exceptionThrown = FALSE;
    
    __try
        SnapCommandLine_Init(&m_commandLine, m_argc, m_argv);
    __catch
        exceptionThrown = TRUE;
        
    CHECK_TRUE(exceptionThrown);
    LONGS_EQUAL(invalidArgumentException, getExceptionCode());
    LONGS_EQUAL(0, m_commandLine.sourceFileCount);
    CHECK(m_commandLine.pSourceFiles == NULL);
    STRCMP_EQUAL(g_usageString, printfSpy_GetLastOutput());
    clearExceptionCode();
}

TEST(SnapCommandLine, OneSourceFilename)
{
    addArg("SOURCE1.S");
    
    SnapCommandLine_Init(&m_commandLine, m_argc, m_argv);
    validateSourceFilesAndNoErrorMessage(m_argv, 1);
}

TEST(SnapCommandLine, TwoSourceFilenames)
{
    addArg("SOURCE1.S");
    addArg("SOURCE2.S");
    
    SnapCommandLine_Init(&m_commandLine, m_argc, m_argv);
    validateSourceFilesAndNoErrorMessage(m_argv, 2);
}
