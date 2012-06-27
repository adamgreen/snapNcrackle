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
    #include "Assembler.h"
    #include "MallocFailureInject.h"
    #include "printfSpy.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(Assembler)
{
    Assembler* m_pAssembler;
    char       m_buffer[128];
    
    void setup()
    {
        clearExceptionCode();
        printfSpy_Construct(128);
        m_pAssembler = NULL;
    }

    void teardown()
    {
        printfSpy_Destruct();
        Assembler_Free(m_pAssembler);
        LONGS_EQUAL(noException, getExceptionCode());
    }
    
    char* dupe(const char* pString)
    {
        strcpy(m_buffer, pString);
        return m_buffer;
    }
};


TEST(Assembler, FailFirstInitAllocation)
{
    MallocFailureInject_Construct(1);
    m_pAssembler = Assembler_CreateFromString(dupe(""));
    MallocFailureInject_Destruct();
    POINTERS_EQUAL(NULL, m_pAssembler);
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}

TEST(Assembler, FailSecondInitAllocation)
{
    MallocFailureInject_Construct(2);
    m_pAssembler = Assembler_CreateFromString(dupe(""));
    MallocFailureInject_Destruct();
    POINTERS_EQUAL(NULL, m_pAssembler);
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}

TEST(Assembler, FailThirdInitAllocation)
{
    MallocFailureInject_Construct(3);
    m_pAssembler = Assembler_CreateFromString(dupe(""));
    MallocFailureInject_Destruct();
    POINTERS_EQUAL(NULL, m_pAssembler);
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}

TEST(Assembler, EmptyString)
{
    m_pAssembler = Assembler_CreateFromString(dupe(""));
    CHECK(m_pAssembler != NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(1, printfSpy_GetCallCount());
}

TEST(Assembler, OneOperator)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ORG $800\r\n"));
    CHECK(m_pAssembler != NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    STRCMP_EQUAL("ORG\r\n", printfSpy_GetLastOutput());
}

TEST(Assembler, TwoOperators)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ORG $800\r\n"
                                                   " INX\r\n"));
    CHECK(m_pAssembler != NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(3, printfSpy_GetCallCount());
}

TEST(Assembler, LinesWithoutOperators)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ORG $800\r\n"
                                                   " INX\r\n"
                                                   "* Comment\r\n"));
    CHECK(m_pAssembler != NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(3, printfSpy_GetCallCount());
}

TEST(Assembler, SameOperatorTwice)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" INX\r\n"
                                                   " INX\r\n"));
    CHECK(m_pAssembler != NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
}

TEST(Assembler, FailSymbolAllocation)
{
    MallocFailureInject_Construct(5);
    m_pAssembler = Assembler_CreateFromString(dupe(" ORG $800\r\n"));
    CHECK(m_pAssembler != NULL);
    Assembler_Run(m_pAssembler);
    MallocFailureInject_Destruct();

    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    LONGS_EQUAL(1, printfSpy_GetCallCount());
    clearExceptionCode();
}
