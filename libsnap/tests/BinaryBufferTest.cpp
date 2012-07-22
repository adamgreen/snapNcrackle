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
        m_pAlloc = BinaryBuffer_Allocate(m_pBinaryBuffer, m_allocSize);
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
    unsigned char* pAlloc = BinaryBuffer_Allocate(m_pBinaryBuffer, 1);
    CHECK_TRUE(NULL != pAlloc);
}

TEST(BinaryBuffer, Allocate2Items)
{
    m_pBinaryBuffer = BinaryBuffer_Create(64*1024);
    unsigned char* pAlloc1 = BinaryBuffer_Allocate(m_pBinaryBuffer, 1);
    unsigned char* pAlloc2 = BinaryBuffer_Allocate(m_pBinaryBuffer, 2);
    CHECK_TRUE(NULL != pAlloc2);
    CHECK_TRUE(pAlloc2 == pAlloc1+1);
}

TEST(BinaryBuffer, FailToAllocateItem)
{
    m_pBinaryBuffer = BinaryBuffer_Create(1);
    unsigned char* pAlloc = BinaryBuffer_Allocate(m_pBinaryBuffer, 2);
    validateOutOfMemoryExceptionThrown();
    POINTERS_EQUAL(NULL, pAlloc);
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
