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
    #include "LineInfo.h"
    #include "MallocFailureInject.h"
    #include "util.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(LineInfo)
{
    LineInfo m_lineInfo;
    
    void setup()
    {
        clearExceptionCode();
        memset(&m_lineInfo, 0xff, sizeof(m_lineInfo));
    }

    void teardown()
    {
        MallocFailureInject_Restore();
        LineInfo_Free(&m_lineInfo);
        LONGS_EQUAL(noException, getExceptionCode());
    }
};


TEST(LineInfo, FailFirstAllocationInInit)
{
    MallocFailureInject_FailAllocation(1);
    LineInfo_Init(&m_lineInfo);
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}

TEST(LineInfo, SaveSmallTextLine)
{
    const char* testLine = "Hello\r\n";
    LineInfo_Init(&m_lineInfo);
    LineInfo_SaveLineText(&m_lineInfo, testLine);
    STRCMP_EQUAL(testLine, m_lineInfo.pLineText);
    CHECK(testLine != m_lineInfo.pLineText);
}

TEST(LineInfo, SaveLargeTextFile)
{
    char testLine[257];
    LineInfo_Init(&m_lineInfo);
    memset(testLine, ' ', sizeof(testLine));
    testLine[ARRAYSIZE(testLine)-1] = '\0';
    LineInfo_SaveLineText(&m_lineInfo, testLine);
    STRCMP_EQUAL(testLine, m_lineInfo.pLineText);
    CHECK(testLine != m_lineInfo.pLineText);
}
