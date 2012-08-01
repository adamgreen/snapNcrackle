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
#include "CrackleCommandLine.h"
#include "CrackleCommandLineTest.h"
#include "util.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


static const char g_usageString[] = "Usage:";


TEST_GROUP(CrackleCommandLine)
{
    const char*        m_argv[10];
    CrackleCommandLine m_commandLine;
    int                m_argc;
    
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
};


TEST(CrackleCommandLine, NoParameters)
{
    m_commandLine = CrackleCommandLine_Init(m_argc, m_argv);
    LONGS_EQUAL(invalidArgumentException, getExceptionCode());
    clearExceptionCode();
    STRCMP_EQUAL(g_usageString, printfSpy_GetLastOutput());
}

TEST(CrackleCommandLine, OneParameter)
{
    addArg("pop1.crackle");
    m_commandLine = CrackleCommandLine_Init(m_argc, m_argv);
    LONGS_EQUAL(invalidArgumentException, getExceptionCode());
    clearExceptionCode();
    STRCMP_EQUAL(g_usageString, printfSpy_GetLastOutput());
}

TEST(CrackleCommandLine, TwoParameter)
{
    addArg("pop1.crackle");
    addArg("pop1.nib");
    m_commandLine = CrackleCommandLine_Init(m_argc, m_argv);
    LONGS_EQUAL(0, printfSpy_GetCallCount());
    STRCMP_EQUAL("pop1.crackle", m_commandLine.pScriptFilename);
    STRCMP_EQUAL("pop1.nib", m_commandLine.pOutputImageFilename);
}

TEST(CrackleCommandLine, OneTooManyParameters)
{
    addArg("pop1.crackle");
    addArg("pop1.nib");
    addArg("one.toomany");
    m_commandLine = CrackleCommandLine_Init(m_argc, m_argv);
    LONGS_EQUAL(invalidArgumentException, getExceptionCode());
    clearExceptionCode();
    STRCMP_EQUAL(g_usageString, printfSpy_GetLastOutput());
}
