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
#include "MallocFailureInject.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(MallocFailureInject)
{
    void setup()
    {
    }

    void teardown()
    {
        MallocFailureInject_Destruct();
    }
};


TEST(MallocFailureInject, FailFirstMalloc)
{
    MallocFailureInject_Construct(1);
    
    void* pTest = __malloc(10);
    POINTERS_EQUAL(NULL, pTest);
}

TEST(MallocFailureInject, FailSecondMalloc)
{
    MallocFailureInject_Construct(2);
    
    void* pFirstMalloc = __malloc(10);
    CHECK(NULL != pFirstMalloc);
    
    void* pSecondMalloc = __malloc(20);
    POINTERS_EQUAL(NULL, pSecondMalloc);
    
    free(pFirstMalloc);
}
