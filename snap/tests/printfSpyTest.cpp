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
    #include "printfSpy.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


const char g_HelloWorld[] = "Hello World!\n";


TEST_GROUP(printfSpy)
{
    int m_Result;
    
    void setup()
    {
        m_Result = -1;
    }

    void teardown()
    {
        printfSpy_Destruct();
    }
  
    void printfCheck(int ExpectedLength, const char* pExpectedString)
    {
        LONGS_EQUAL(ExpectedLength, m_Result);
        STRCMP_EQUAL(pExpectedString, printfSpy_GetLastOutput());
        LONGS_EQUAL(1, printfSpy_GetCallCount());
    }

    char* CreateCheckBuffer(size_t BufferSize)
    {
        char* pCheckString = NULL;
    
        pCheckString = (char*)malloc(BufferSize + 1);
        CHECK(NULL != pCheckString);
    
        strncpy(pCheckString, g_HelloWorld, BufferSize);
        pCheckString[BufferSize] = '\0';
    
        return pCheckString;
    }

    void printfCheckHelloWorldWithBufferOfSize(size_t BufferSize)
    {
        printfSpy_Construct(BufferSize);
        m_Result = printfSpy_printf(g_HelloWorld);

        char* pCheckString = CreateCheckBuffer(BufferSize);

        printfCheck(sizeof(g_HelloWorld)-1, pCheckString);

        free(pCheckString);
    }
};


TEST(printfSpy, BufferSize0)
{
    printfCheckHelloWorldWithBufferOfSize(0);
}


TEST(printfSpy, BufferSize1)
{
    printfCheckHelloWorldWithBufferOfSize(1);
}

TEST(printfSpy, BufferSizeMinus1)
{
    printfCheckHelloWorldWithBufferOfSize(sizeof(g_HelloWorld)-2);
}

TEST(printfSpy, BufferSizeExact)
{
    printfCheckHelloWorldWithBufferOfSize(sizeof(g_HelloWorld)-1);
}

TEST(printfSpy, BufferSizePlus1)
{
    printfCheckHelloWorldWithBufferOfSize(sizeof(g_HelloWorld));
}

TEST(printfSpy, WithFormatting)
{
    printfSpy_Construct(10);
    m_Result = printfSpy_printf("Hello %s\n", "World");

    printfCheck(12, "Hello Worl");
}

TEST(printfSpy, TwoCall)
{
    printfSpy_printf("Line 1\r\n");
    printfSpy_printf("Line 2\r\n");
    LONGS_EQUAL(2, printfSpy_GetCallCount());
}
