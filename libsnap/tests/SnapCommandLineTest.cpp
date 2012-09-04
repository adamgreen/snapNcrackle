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
#include "SnapCommandLine.h"
#include "SnapCommandLineTest.h"
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
    
    void validateSourceFilesAndNoErrorMessage(const char* pSourceFilename)
    {
        validateNoErrorMessageWasDisplayed();
        validateSourceFiles(pSourceFilename);
    }
    
    void validateNoErrorMessageWasDisplayed(void)
    {
        STRCMP_EQUAL("", printfSpy_GetLastOutput());
    }
    
    void validateSourceFiles(const char* pSourceFilename)
    {
        STRCMP_EQUAL(pSourceFilename, m_commandLine.pSourceFilename);
    }
    
    void validateInvalidArgumentExceptionAndUsageString(void)
    {
        LONGS_EQUAL(invalidArgumentException, getExceptionCode());
        STRCMP_EQUAL(g_usageString, printfSpy_GetLastOutput());
        clearExceptionCode();
    }
};


TEST(SnapCommandLine, NoParameters)
{
    __try_and_catch( SnapCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateInvalidArgumentExceptionAndUsageString();
    CHECK(m_commandLine.pSourceFilename == NULL);
}

TEST(SnapCommandLine, OneSourceFilename)
{
    addArg("SOURCE1.S");
    
    SnapCommandLine_Init(&m_commandLine, m_argc, m_argv);
    validateSourceFilesAndNoErrorMessage("SOURCE1.S");
}

TEST(SnapCommandLine, FailOnTwoSourceFilenames)
{
    addArg("SOURCE1.S");
    addArg("SOURCE2.S");
    
    __try_and_catch( SnapCommandLine_Init(&m_commandLine, m_argc, m_argv) );
    validateInvalidArgumentExceptionAndUsageString();
}
