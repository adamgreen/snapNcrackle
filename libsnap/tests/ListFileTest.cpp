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
    #include "ListFile.h"
    #include "MallocFailureInject.h"
    #include "printfSpy.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(ListFile)
{
    ListFile* m_pListFile;
    
    void setup()
    {
        clearExceptionCode();
        printfSpy_Hook(128);
        m_pListFile = NULL;
    }

    void teardown()
    {
        MallocFailureInject_Restore();
        ListFile_Free(m_pListFile);
        LONGS_EQUAL(noException, getExceptionCode());
        printfSpy_Unhook();
    }
};


TEST(ListFile, FailFirstAllocDuringCreate)
{
    MallocFailureInject_FailAllocation(1);
    m_pListFile = ListFile_Create(stdout);
    POINTERS_EQUAL(NULL, m_pListFile);
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}

IGNORE_TEST(ListFile, FailSecondAllocDuringCreate)
{
    MallocFailureInject_FailAllocation(2);
    m_pListFile = ListFile_Create(stdout);
    CHECK(m_pListFile != NULL);
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}

TEST(ListFile, SaveLineText)
{
    m_pListFile = ListFile_Create(stdout);
    ListFile_SaveLineText(m_pListFile, "* Full line comment.");
}

TEST(ListFile, OutputLine)
{
    m_pListFile = ListFile_Create(stdout);
    ListFile_SaveLineText(m_pListFile, "* Full line comment.");
    ListFile_OutputLine(m_pListFile);
    POINTERS_EQUAL(stdout, printfSpy_GetLastFile());
    STRCMP_EQUAL("0000: 00 00          1 * Full line comment.\n", printfSpy_GetLastOutput());
}

/*
    Treat all lines as comments.
    XXXX: XX XX XXXX NNNNN Text
*/
