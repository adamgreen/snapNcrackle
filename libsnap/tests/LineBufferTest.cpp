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
    #include "LineBuffer.h"
    #include "MallocFailureInject.h"
    #include "util.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(LineBuffer)
{
    LineBuffer* m_pLineBuffer;
    
    void setup()
    {
        clearExceptionCode();
        m_pLineBuffer = NULL;
    }

    void teardown()
    {
        MallocFailureInject_Restore();
        LineBuffer_Free(m_pLineBuffer);
        LONGS_EQUAL(noException, getExceptionCode());
    }
};


TEST(LineBuffer, Init)
{
    m_pLineBuffer = LineBuffer_Init();
    CHECK(m_pLineBuffer);
    STRCMP_EQUAL("", LineBuffer_Get(m_pLineBuffer));
}

TEST(LineBuffer, FailFirstAllocationInInit)
{
    MallocFailureInject_FailAllocation(1);
    m_pLineBuffer = LineBuffer_Init();
    POINTERS_EQUAL(NULL, m_pLineBuffer);
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}

TEST(LineBuffer, FailSecondAllocationInInit)
{
    MallocFailureInject_FailAllocation(2);
    m_pLineBuffer = LineBuffer_Init();
    POINTERS_EQUAL(NULL, m_pLineBuffer);
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}

TEST(LineBuffer, SetSmallText)
{
    const char* testLine = "Hello\r\n";
    m_pLineBuffer = LineBuffer_Init();
    LineBuffer_Set(m_pLineBuffer, testLine);
    STRCMP_EQUAL(testLine, LineBuffer_Get(m_pLineBuffer));
    CHECK(testLine != LineBuffer_Get(m_pLineBuffer));
}

TEST(LineBuffer, SetLargeText)
{
    char testLine[257];
    m_pLineBuffer = LineBuffer_Init();
    memset(testLine, ' ', sizeof(testLine));
    testLine[ARRAYSIZE(testLine)-1] = '\0';
    LineBuffer_Set(m_pLineBuffer, testLine);
    STRCMP_EQUAL(testLine, LineBuffer_Get(m_pLineBuffer));
    CHECK(testLine != LineBuffer_Get(m_pLineBuffer));
}

TEST(LineBuffer, FailAllocationDuringLargeTextSet)
{
    char testLine[257];
    m_pLineBuffer = LineBuffer_Init();
    memset(testLine, ' ', sizeof(testLine));
    testLine[ARRAYSIZE(testLine)-1] = '\0';
    MallocFailureInject_FailAllocation(1);
    LineBuffer_Set(m_pLineBuffer, testLine);
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    STRCMP_EQUAL("", LineBuffer_Get(m_pLineBuffer));
    clearExceptionCode();
}
