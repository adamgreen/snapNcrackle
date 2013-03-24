/*  Copyright (C) 2013  Adam Green (https://github.com/adamgreen)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
#include <string.h>

// Include headers from C modules under test.
extern "C"
{
    #include "ByteBuffer.h"
    #include "MallocFailureInject.h"
    #include "FileFailureInject.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"

static const char g_TestFilename[] = "ByteBufferTest.bin";

TEST_GROUP(ByteBuffer)
{
    ByteBuffer     m_buffer;
    FILE*          m_pFile;
    unsigned char* m_pFileData;
    
    void setup()
    {
        memset(&m_buffer, 0, sizeof(m_buffer));
        m_pFile = NULL;
        m_pFileData = NULL;
    }

    void teardown()
    {
        LONGS_EQUAL(noException, getExceptionCode());
        MallocFailureInject_Restore();
        freadRestore();
        fwriteRestore();
        ByteBuffer_Free(&m_buffer);
        free(m_pFileData);
        if (m_pFile)
            fclose(m_pFile);
        remove(g_TestFilename);
    }
    
    void writeBufferToFile()
    {
        m_pFile = fopen(g_TestFilename, "w");
        CHECK(NULL != m_pFile);

        ByteBuffer_WriteToFile(&m_buffer, m_pFile);

        fclose(m_pFile);
        m_pFile = NULL;
    }
    
    void validateFileContainsBufferContents()
    {
        m_pFile = fopen(g_TestFilename, "r");
        LONGS_EQUAL(m_buffer.bufferSize, getFileSize(m_pFile));
        m_pFileData = (unsigned char*)malloc(m_buffer.bufferSize);
        fread(m_pFileData, 1, m_buffer.bufferSize, m_pFile);

        CHECK(0 == memcmp(m_buffer.pBuffer, m_pFileData, m_buffer.bufferSize));
        
        fclose(m_pFile);
        m_pFile = NULL;
    }
    
    void createTestFile(const char* pData, size_t dataSize)
    {
        m_pFile = fopen(g_TestFilename, "w");
        CHECK(NULL != m_pFile);

        LONGS_EQUAL(dataSize, fwrite(pData, 1, dataSize, m_pFile));

        fclose(m_pFile);
        m_pFile = NULL;
    }
    
    void readBufferFromFile()
    {
        m_pFile = fopen(g_TestFilename, "r");
        CHECK(NULL != m_pFile);

        ByteBuffer_ReadFromFile(&m_buffer, m_pFile);

        fclose(m_pFile);
        m_pFile = NULL;
    }
    
    void readPartialBufferFromFile(size_t bytesToRead)
    {
        m_pFile = fopen(g_TestFilename, "r");
        CHECK(NULL != m_pFile);

        ByteBuffer_ReadPartialFromFile(&m_buffer, bytesToRead, m_pFile);

        fclose(m_pFile);
        m_pFile = NULL;
    }

    long getFileSize(FILE* pFile)
    {
        fseek(pFile, 0, SEEK_END);
        long size = ftell(pFile);
        fseek(pFile, 0, SEEK_SET);
        return size;
    }
    
    void validateFileExceptionThrown()
    {
        LONGS_EQUAL(fileException, getExceptionCode());
        clearExceptionCode();
    }
};


TEST(ByteBuffer, InitZeroByteBuffer)
{
    ByteBuffer_Allocate(&m_buffer, 0);
    CHECK(NULL != m_buffer.pBuffer);
    LONGS_EQUAL(0, m_buffer.bufferSize);
}

TEST(ByteBuffer, FailInit)
{
    MallocFailureInject_FailAllocation(1);
    __try_and_catch( ByteBuffer_Allocate(&m_buffer, 0) );
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    POINTERS_EQUAL(NULL, m_buffer.pBuffer);
    LONGS_EQUAL(0, m_buffer.bufferSize);
    clearExceptionCode();
}

TEST(ByteBuffer, InitTenByteBuffer)
{
    ByteBuffer_Allocate(&m_buffer, 10);
    CHECK(NULL != m_buffer.pBuffer);
    LONGS_EQUAL(10, m_buffer.bufferSize);
}

TEST(ByteBuffer, AttemptToFreeNullByteBuffer)
{
    ByteBuffer_Free(NULL);
}

TEST(ByteBuffer, AllocateBufferMultipleTimes)
{
    ByteBuffer_Allocate(&m_buffer, 10);
    CHECK(NULL != m_buffer.pBuffer);
    LONGS_EQUAL(10, m_buffer.bufferSize);

    ByteBuffer_Allocate(&m_buffer, 15);
    CHECK(NULL != m_buffer.pBuffer);
    LONGS_EQUAL(15, m_buffer.bufferSize);
}

TEST(ByteBuffer, WriteToFile)
{
    ByteBuffer_Allocate(&m_buffer, 10);
    memset(m_buffer.pBuffer, 0xa5, m_buffer.bufferSize);
    
    writeBufferToFile();
    validateFileContainsBufferContents();
}

TEST(ByteBuffer, FailWriteToFile)
{
    ByteBuffer_Allocate(&m_buffer, 10);
    memset(m_buffer.pBuffer, 0xa5, m_buffer.bufferSize);
    
    fwriteFail(m_buffer.bufferSize - 1);
        __try_and_catch( writeBufferToFile() );
    fwriteRestore();
    validateFileExceptionThrown();
}

TEST(ByteBuffer, ReadFromFile)
{
    static const char testData[] = "\xa5\x5a\a5\5a\a5";
    createTestFile(testData, sizeof(testData));

    ByteBuffer_Allocate(&m_buffer, sizeof(testData));
    readBufferFromFile();
    CHECK(0 == memcmp(testData, m_buffer.pBuffer, sizeof(testData)));
}

TEST(ByteBuffer, FailReadFromFile)
{
    ByteBuffer_Allocate(&m_buffer, 10);
    freadFail(0);
        __try_and_catch( ByteBuffer_ReadFromFile(&m_buffer, NULL) );
    freadRestore();
    validateFileExceptionThrown();
}

TEST(ByteBuffer, ReadPartialFromFileAndRestShouldBeZeroFilled)
{
    static const char testData[5] = { 0xa5, 0xa5, 0xa5, 0xa5, 0xa5 };
    createTestFile(testData, sizeof(testData));

    ByteBuffer_Allocate(&m_buffer, sizeof(testData));
    readPartialBufferFromFile(sizeof(testData)-1);
    CHECK(0 == memcmp(testData, m_buffer.pBuffer, sizeof(testData)-1));
    LONGS_EQUAL(0x00, m_buffer.pBuffer[sizeof(testData)-1]);
}

TEST(ByteBuffer, FailReadPartialFromFile)
{
    ByteBuffer_Allocate(&m_buffer, 10);
    freadFail(0);
        __try_and_catch( ByteBuffer_ReadPartialFromFile(&m_buffer, 5, NULL) );
    freadRestore();
    validateFileExceptionThrown();
}

TEST(ByteBuffer, FailByReadingTooMuchFromReadPartialFromFile)
{
    ByteBuffer_Allocate(&m_buffer, 10);
    __try_and_catch( ByteBuffer_ReadPartialFromFile(&m_buffer, 11, NULL) );
    LONGS_EQUAL(invalidArgumentException, getExceptionCode());
    clearExceptionCode();
}
