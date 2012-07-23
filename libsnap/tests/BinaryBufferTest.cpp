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
    #include "BinaryBuffer.h"
    #include "MallocFailureInject.h"
    #include "FileFailureInject.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


static const char* g_filename = "BinaryBufferTest.test";

TEST_GROUP(BinaryBuffer)
{
    BinaryBuffer*   m_pBinaryBuffer;
    unsigned char*  m_pAlloc;
    size_t          m_allocSize;
    
    void setup()
    {
        clearExceptionCode();
        m_pBinaryBuffer = NULL;
        m_pAlloc = NULL;
        m_allocSize = 0;
    }

    void teardown()
    {
        MallocFailureInject_Restore();
        fopenRestore();
        fwriteRestore();
        LONGS_EQUAL(noException, getExceptionCode());
        BinaryBuffer_Free(m_pBinaryBuffer);
        remove(g_filename);
    }
    
    void validateOutOfMemoryExceptionThrown()
    {
        LONGS_EQUAL(outOfMemoryException, getExceptionCode());
        clearExceptionCode();
    }
    
    void placeSomeDataInBuffer()
    {
        m_allocSize = 2;
        m_pBinaryBuffer = BinaryBuffer_Create(64*1024);
        m_pAlloc = BinaryBuffer_Alloc(m_pBinaryBuffer, m_allocSize);
        m_pAlloc[0] = 0x00;
        m_pAlloc[1] = 0xff;
    }
};


TEST(BinaryBuffer, FailFirstAllocationDuringCreate)
{
    MallocFailureInject_FailAllocation(1);
    m_pBinaryBuffer = BinaryBuffer_Create(64*1024);
    POINTERS_EQUAL(NULL, m_pBinaryBuffer);
    validateOutOfMemoryExceptionThrown();
}

TEST(BinaryBuffer, FailSecondAllocationDuringCreate)
{
    MallocFailureInject_FailAllocation(2);
    m_pBinaryBuffer = BinaryBuffer_Create(64*1024);
    LONGS_EQUAL(NULL, m_pBinaryBuffer);
    validateOutOfMemoryExceptionThrown();
}

TEST(BinaryBuffer, Allocate1Item)
{
    m_pBinaryBuffer = BinaryBuffer_Create(64*1024);
    CHECK_TRUE(NULL != m_pBinaryBuffer);    
    unsigned char* pAlloc = BinaryBuffer_Alloc(m_pBinaryBuffer, 1);
    CHECK_TRUE(NULL != pAlloc);
}

TEST(BinaryBuffer, Allocate2Items)
{
    m_pBinaryBuffer = BinaryBuffer_Create(64);
    unsigned char* pAlloc1 = BinaryBuffer_Alloc(m_pBinaryBuffer, 1);
    unsigned char* pAlloc2 = BinaryBuffer_Alloc(m_pBinaryBuffer, 2);
    CHECK_TRUE(NULL != pAlloc2);
    CHECK_TRUE(pAlloc2 == pAlloc1+1);
}

TEST(BinaryBuffer, FailToAllocateItem)
{
    m_pBinaryBuffer = BinaryBuffer_Create(1);
    unsigned char* pAlloc = BinaryBuffer_Alloc(m_pBinaryBuffer, 2);
    validateOutOfMemoryExceptionThrown();
    POINTERS_EQUAL(NULL, pAlloc);
}

TEST(BinaryBuffer, ReallocFromNULLShouldBeSameAsAlloc)
{
    m_pBinaryBuffer = BinaryBuffer_Create(64);
    unsigned char* pAlloc = BinaryBuffer_Realloc(m_pBinaryBuffer, NULL, 1);
    CHECK_TRUE(NULL != pAlloc);
}

TEST(BinaryBuffer, ReallocToGrowBufferByOneByte)
{
    m_pBinaryBuffer = BinaryBuffer_Create(64);
    unsigned char* pAlloc1 = BinaryBuffer_Alloc(m_pBinaryBuffer, 1);
    unsigned char* pAlloc2 = BinaryBuffer_Realloc(m_pBinaryBuffer, pAlloc1, 2);
    CHECK_TRUE(pAlloc1 == pAlloc2);
    unsigned char* pAlloc3 = BinaryBuffer_Alloc(m_pBinaryBuffer, 1);
    CHECK_TRUE(pAlloc3 == pAlloc2 + 2);
}

TEST(BinaryBuffer, FailReallocBySpecifyingPointerOtherThanLastAllocated)
{
    m_pBinaryBuffer = BinaryBuffer_Create(64);
    unsigned char* pAlloc1 = BinaryBuffer_Alloc(m_pBinaryBuffer, 1);
                             BinaryBuffer_Alloc(m_pBinaryBuffer, 1);
    unsigned char* pAlloc3 = BinaryBuffer_Realloc(m_pBinaryBuffer, pAlloc1, 2);
    POINTERS_EQUAL(NULL, pAlloc3);
    LONGS_EQUAL(invalidArgumentException, getExceptionCode());
    clearExceptionCode();
}

TEST(BinaryBuffer, ForceFirstAllocToFail)
{
    m_pBinaryBuffer = BinaryBuffer_Create(64);
    BinaryBuffer_FailAllocation(m_pBinaryBuffer, 1);
    unsigned char* pAlloc1 = BinaryBuffer_Alloc(m_pBinaryBuffer, 1);
    POINTERS_EQUAL(NULL, pAlloc1);
    validateOutOfMemoryExceptionThrown();
}

TEST(BinaryBuffer, WriteBufferToDisk)
{
    char               testBuffer[2];
    
    placeSomeDataInBuffer();
    
    BinaryBuffer_WriteToFile(m_pBinaryBuffer, g_filename);
    FILE* pFile = fopen(g_filename, "r");
    CHECK_TRUE(pFile != NULL);
    LONGS_EQUAL(2, fread(testBuffer, 1, 2, pFile));
    CHECK_TRUE(0 == memcmp(m_pAlloc, testBuffer, sizeof(testBuffer)));
    fclose(pFile);
}

TEST(BinaryBuffer, FailFOpenDuringWriteToFile)
{
    placeSomeDataInBuffer();
    
    fopenFail(NULL);
    BinaryBuffer_WriteToFile(m_pBinaryBuffer, g_filename);
    LONGS_EQUAL(fileException, getExceptionCode());
    clearExceptionCode();
}

TEST(BinaryBuffer, FailFWriteDuringWriteToFile)
{
    placeSomeDataInBuffer();
    
    fwriteFail(0);
    BinaryBuffer_WriteToFile(m_pBinaryBuffer, g_filename);
    LONGS_EQUAL(fileException, getExceptionCode());
    clearExceptionCode();
}

TEST(BinaryBuffer, SetOriginBeforeAnyAllocations)
{
    m_pBinaryBuffer = BinaryBuffer_Create(64);
    LONGS_EQUAL(0, BinaryBuffer_GetOrigin(m_pBinaryBuffer));
    BinaryBuffer_SetOrigin(m_pBinaryBuffer, 0x8000);
    LONGS_EQUAL(0x8000, BinaryBuffer_GetOrigin(m_pBinaryBuffer));
}

TEST(BinaryBuffer, WritePartialBufferToDisk)
{
    char               readBuffer[4];
    
    placeSomeDataInBuffer();
    BinaryBuffer_SetOrigin(m_pBinaryBuffer, 0x8000);
    unsigned char* pAlloc = BinaryBuffer_Alloc(m_pBinaryBuffer, 2);
    pAlloc[0] = 0xa5;
    pAlloc[1] = 0x5a;
    
    BinaryBuffer_WriteToFile(m_pBinaryBuffer, g_filename);
    FILE* pFile = fopen(g_filename, "r");
    CHECK_TRUE(pFile != NULL);
    LONGS_EQUAL(2, fread(readBuffer, 1, 4, pFile));
    CHECK_TRUE(0 == memcmp(pAlloc, readBuffer, 2));
    fclose(pFile);
}
