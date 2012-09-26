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
#ifndef _ASSEMBLER_BASE_TEST_H_
#define _ASSEMBLER_BASE_TEST_H_

// Include headers from C modules under test.
#include <stdlib.h>
#include <assert.h>

extern "C"
{
    #include "SnapCommandLine.h"
    #include "Assembler.h"
    #include "../src/AssemblerPriv.h"
    #include "MallocFailureInject.h"
    #include "FileFailureInject.h"
    #include "printfSpy.h"
    #include "BinaryBuffer.h"
    #include "util.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


static const char* g_sourceFilename = "AssemblerTest.S";
static const char* g_objectFilename = "AssemblerTest.sav";
static const char* g_listFilename = "AssemblerTest.lst";

TEST_BASE(AssemblerBase)
{
    Assembler*          m_pAssembler;
    const char*         m_argv[10];
    FILE*               m_pFile;
    char*               m_pReadBuffer;
    SnapCommandLine     m_commandLine;
    AssemblerInitParams m_initParams;
    int                 m_argc;
    char                m_buffer[512];
    
    void setup()
    {
        clearExceptionCode();
        printfSpy_Hook(512);
        memset(&m_initParams, 0, sizeof(m_initParams));
        m_pAssembler = NULL;
        m_pFile = NULL;
        m_pReadBuffer = NULL;
    }

    void teardown()
    {
        MallocFailureInject_Restore();
        printfSpy_Unhook();
        Assembler_Free(m_pAssembler);
        if (m_pFile)
            fclose(m_pFile);
        free(m_pReadBuffer);
        remove(g_sourceFilename);
        remove(g_objectFilename);
        remove(g_listFilename);
        LONGS_EQUAL(noException, getExceptionCode());
    }
    
    char* dupe(const char* pString)
    {
        assert ( strlen(pString) < sizeof(m_buffer) );
        strcpy(m_buffer, pString);
        return m_buffer;
    }
    
    void createSourceFile(const char* pString)
    {
        createThisSourceFile(g_sourceFilename, pString);
    }

    void createThisSourceFile(const char* pFilename, const char* pString)
    {
        FILE* pFile = fopen(pFilename, "w");
        fwrite(pString, 1, strlen(pString), pFile);
        fclose(pFile);
    }

    void addArg(const char* pArg)
    {
        CHECK(m_argc < (int)ARRAYSIZE(m_argv));
        m_argv[m_argc++] = pArg;
    }
    
    void validateFileNotFoundExceptionThrown()
    {
        validateExceptionThrown(fileNotFoundException);
    }
    
    void validateOutOfMemoryExceptionThrown()
    {
        validateExceptionThrown(outOfMemoryException);
    }
    
    void validateExceptionThrown(int expectedException)
    {
        LONGS_EQUAL(expectedException, getExceptionCode());
        clearExceptionCode();
    }
    
    void runAssemblerAndValidateOutputIs(const char* pExpectedOutput)
    {
        runAssemblerAndValidateLastLineIs(pExpectedOutput, 1);
    }
    
    void runAssemblerAndValidateLastLineIs(const char* pExpectedOutput, int expectedPrintfCalls)
    {
        Assembler_Run(m_pAssembler);
        validateSuccessfulOutput(pExpectedOutput, expectedPrintfCalls);
        LONGS_EQUAL(0, Assembler_GetErrorCount(m_pAssembler) );
    }
    
    void validateSuccessfulOutput(const char* pExpectedListOutput, long expectedPrintfCalls = 1)
    {
        STRCMP_EQUAL(pExpectedListOutput, printfSpy_GetLastOutput());
        LONGS_EQUAL(expectedPrintfCalls, printfSpy_GetCallCount());
    }
    
    void runAssemblerAndValidateLastTwoLinesOfOutputAre(const char* pExpectedOutput1, const char* pExpectedOutput2, int expectedPrintfCalls = 2)
    {
        Assembler_Run(m_pAssembler);
        LONGS_EQUAL(expectedPrintfCalls, printfSpy_GetCallCount());
        STRCMP_EQUAL(pExpectedOutput1, printfSpy_GetPreviousOutput());
        STRCMP_EQUAL(pExpectedOutput2, printfSpy_GetLastOutput());
        LONGS_EQUAL(0, Assembler_GetErrorCount(m_pAssembler) );
    }
    
    void runAssemblerAndValidateFailure(const char* pExpectedFailureMessage, const char* pExpectedListOutput, long expectedPrintfCalls = 2)
    {
        Assembler_Run(m_pAssembler);
        validateFailureOutput(pExpectedFailureMessage, pExpectedListOutput, expectedPrintfCalls);
        LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler) );
    }
    
    void validateFailureOutput(const char* pExpectedErrorOutput, const char* pExpectedListOutput, int expectedPrintfCalls = 2)
    {
        STRCMP_EQUAL(pExpectedErrorOutput, printfSpy_GetLastErrorOutput());
        STRCMP_EQUAL(pExpectedListOutput, printfSpy_GetLastOutput());
        LONGS_EQUAL(expectedPrintfCalls, printfSpy_GetCallCount());
    }
    
    void validateLineInfo(const LineInfo*      pLineInfo, 
                          unsigned short       expectedAddress, 
                          size_t               expectedMachineCodeSize,
                          const char*          pExpectedMachineCode)
    {
        LONGS_EQUAL(expectedMachineCodeSize, pLineInfo->machineCodeSize);
        CHECK_TRUE(0 == memcmp(pLineInfo->pMachineCode, pExpectedMachineCode, expectedMachineCodeSize));
        LONGS_EQUAL(expectedAddress, pLineInfo->address);
    }
    
    void validateObjectFileContains(unsigned short expectedAddress, const char* pExpectedContent, long expectedContentSize)
    {
        SavFileHeader header;
        
        m_pFile = fopen(g_objectFilename, "r");
        CHECK(m_pFile != NULL);
        LONGS_EQUAL(expectedContentSize + sizeof(header), getFileSize(m_pFile));
        
        LONGS_EQUAL(sizeof(header), fread(&header, 1, sizeof(header), m_pFile));
        CHECK(0 == memcmp(header.signature, BINARY_BUFFER_SAV_SIGNATURE, sizeof(header.signature)));
        LONGS_EQUAL(expectedAddress, header.address);
        LONGS_EQUAL(expectedContentSize, header.length);
        
        m_pReadBuffer = (char*)malloc(expectedContentSize);
        CHECK(m_pReadBuffer != NULL);
        LONGS_EQUAL(expectedContentSize, fread(m_pReadBuffer, 1, expectedContentSize, m_pFile));
        CHECK(0 == memcmp(pExpectedContent, m_pReadBuffer, expectedContentSize));
        
        free(m_pReadBuffer);
        m_pReadBuffer = NULL;
        fclose(m_pFile);
        m_pFile = NULL;
    }
    
    void validateRW18ObjectFileContains(const char*    pFilename,
                                        unsigned short expectedSide,
                                        unsigned short expectedTrack,
                                        unsigned short expectedOffset,
                                        const char*    pExpectedContent, 
                                        long           expectedContentSize)
    {
        RW18SavFileHeader header;
        
        m_pFile = fopen(pFilename, "r");
        CHECK(m_pFile != NULL);
        LONGS_EQUAL(expectedContentSize + sizeof(header), getFileSize(m_pFile));
        
        LONGS_EQUAL(sizeof(header), fread(&header, 1, sizeof(header), m_pFile));
        CHECK(0 == memcmp(header.signature, BINARY_BUFFER_RW18SAV_SIGNATURE, sizeof(header.signature)));
        LONGS_EQUAL(expectedSide, header.side);
        LONGS_EQUAL(expectedTrack, header.track);
        LONGS_EQUAL(expectedOffset, header.offset);
        LONGS_EQUAL(expectedContentSize, header.length);
        
        m_pReadBuffer = (char*)malloc(expectedContentSize);
        CHECK(m_pReadBuffer != NULL);
        LONGS_EQUAL(expectedContentSize, fread(m_pReadBuffer, 1, expectedContentSize, m_pFile));
        CHECK(0 == memcmp(pExpectedContent, m_pReadBuffer, expectedContentSize));
        free(m_pReadBuffer);
        m_pReadBuffer = NULL;
        fclose(m_pFile);
        m_pFile = NULL;
    }
    
    void validateListFileContains(const char* pExpectedContent, long expectedContentSize)
    {
        m_pFile = fopen(g_listFilename, "r");
        CHECK(m_pFile != NULL);
        LONGS_EQUAL(expectedContentSize, getFileSize(m_pFile));
        
        m_pReadBuffer = (char*)malloc(expectedContentSize);
        CHECK(m_pReadBuffer != NULL);
        LONGS_EQUAL(expectedContentSize, fread(m_pReadBuffer, 1, expectedContentSize, m_pFile));
        CHECK(0 == memcmp(pExpectedContent, m_pReadBuffer, expectedContentSize));
        
        free(m_pReadBuffer);
        m_pReadBuffer = NULL;
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
};

#endif /* _ASSEMBLER_BASE_TEST_H_ */
