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
#include <stdlib.h>
extern "C"
{
    #include "CommandLine.h"
    #include "Assembler.h"
    #include "../src/AssemblerPriv.h"
    #include "MallocFailureInject.h"
    #include "printfSpy.h"
    #include "util.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


const char* g_sourceFilename = "AssemblerTest.S";

TEST_GROUP(Assembler)
{
    Assembler*    m_pAssembler;
    const char*   m_argv[10];
    CommandLine   m_commandLine;
    int           m_argc;
    int           m_isInvalidMode;
    int           m_isZeroPageTreatedAsAbsolute;
    char          m_buffer[128];
    unsigned char m_expectedOpcode;
    
    void setup()
    {
        clearExceptionCode();
        printfSpy_Hook(512);
        m_pAssembler = NULL;
    }

    void teardown()
    {
        MallocFailureInject_Restore();
        printfSpy_Unhook();
        Assembler_Free(m_pAssembler);
        remove(g_sourceFilename);
        LONGS_EQUAL(noException, getExceptionCode());
    }
    
    char* dupe(const char* pString)
    {
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
        POINTERS_EQUAL(NULL, m_pAssembler);
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
        LONGS_EQUAL(expectedPrintfCalls, printfSpy_GetCallCount());
        STRCMP_EQUAL(pExpectedOutput, printfSpy_GetLastOutput());
        LONGS_EQUAL(0, Assembler_GetErrorCount(m_pAssembler) );
    }
    
    void runAssemblerAndValidateOutputIsTwoLinesOf(const char* pExpectedOutput1, const char* pExpectedOutput2, int expectedPrintfCalls = 2)
    {
        Assembler_Run(m_pAssembler);
        LONGS_EQUAL(expectedPrintfCalls, printfSpy_GetCallCount());
        STRCMP_EQUAL(pExpectedOutput1, printfSpy_GetPreviousOutput());
        STRCMP_EQUAL(pExpectedOutput2, printfSpy_GetLastOutput());
        LONGS_EQUAL(0, Assembler_GetErrorCount(m_pAssembler) );
    }
    
    void runAssemblerAndValidateFailure(const char* pExpectedFailureMessage, const char* pExpectedListOutput)
    {
        runAssemblerAndValidateFailure(pExpectedFailureMessage, pExpectedListOutput, 2);
    }

    void runAssemblerAndValidateFailure(const char* pExpectedFailureMessage, const char* pExpectedListOutput, long expectedPrintfCalls)
    {
        Assembler_Run(m_pAssembler);
        LONGS_EQUAL(expectedPrintfCalls, printfSpy_GetCallCount());
        STRCMP_EQUAL(pExpectedFailureMessage, printfSpy_GetLastErrorOutput());
        STRCMP_EQUAL(pExpectedListOutput, printfSpy_GetLastOutput());
        LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler) );
    }
    
    void test6502Instruction(const char* pInstruction, const char* pInstructionTableEntry)
    {
        const char*        pCurr = pInstructionTableEntry;
    
        CHECK_TRUE(strlen(pInstructionTableEntry) >= 14*3-1);
        
        pCurr = parseEntryAndAdvanceToNextEntry(pCurr);
        testImmediateAddressing(pInstruction);
        
        pCurr = parseEntryAndAdvanceToNextEntry(pCurr);
        testAbsoluteAddressing(pInstruction);
        
        pCurr = parseEntryAndAdvanceToNextEntry(pCurr);
        testZeroPageAddressing(pInstruction);
        
        pCurr = parseEntryAndAdvanceToNextEntry(pCurr);
        testImpliedAddressing(pInstruction);

        pCurr = parseEntryAndAdvanceToNextEntry(pCurr);
        testZeroPageIndexedIndirectAddressing(pInstruction);

        pCurr = parseEntryAndAdvanceToNextEntry(pCurr);
        testIndirectIndexedAddressing(pInstruction);
        
        pCurr = parseEntryAndAdvanceToNextEntry(pCurr);
        testZeroPageIndexedXAddressing(pInstruction);
        
        pCurr = parseEntryAndAdvanceToNextEntry(pCurr);
        testZeroPageIndexedYAddressing(pInstruction);
        
        pCurr = parseEntryAndAdvanceToNextEntry(pCurr);
        testAbsoluteIndexedXAddressing(pInstruction);
        
        pCurr = parseEntryAndAdvanceToNextEntry(pCurr);
        testAbsoluteIndexedYAddressing(pInstruction);
        
        pCurr = parseEntryAndAdvanceToNextEntry(pCurr);
        testRelativeAddressing(pInstruction);
        
        pCurr = parseEntryAndAdvanceToNextEntry(pCurr);
        testAbsoluteIndirectAddressing(pInstruction);

        pCurr = parseEntryAndAdvanceToNextEntry(pCurr);
        testAbsoluteIndexedIndirectAddressing(pInstruction);
        
        pCurr = parseEntryAndAdvanceToNextEntry(pCurr);
        testZeroPageIndirectAddressing(pInstruction);
        
        CHECK_TRUE(*pCurr == '\0');
    }
    
    const char* parseEntryAndAdvanceToNextEntry(const char* pCurr)
    {
        m_isInvalidMode = FALSE;
        m_isZeroPageTreatedAsAbsolute = FALSE;
        if (0 == strncmp(pCurr, "XX", 2))
            m_isInvalidMode = TRUE;
        else
        {
            if (*pCurr == '^')
            {
                m_isZeroPageTreatedAsAbsolute = TRUE;
                pCurr++;
            }

            m_expectedOpcode = strtoul(pCurr, NULL, 16);
        }
            
        if (pCurr[2] == ',')
            return pCurr + 3;
        else
            return pCurr + 2;
    }
    
    void testImmediateAddressing(const char* pInstruction)
    {
        char               testString[64];
        char               expectedListOutput[128];
        char               expectedErrorOutput[128];

        CHECK_FALSE(m_isZeroPageTreatedAsAbsolute);
        
        sprintf(testString, " %s #$ff\n", pInstruction);
        runAssembler(testString);
        if (!m_isInvalidMode)
        {
            sprintf(expectedListOutput, "0000: %02X FF        1 %s\n", m_expectedOpcode, testString);
            validateSuccessfulOutput(expectedListOutput);
        }
        else
        {
            sprintf(expectedErrorOutput, "filename:1: error: Addressing mode of '#$ff' is not supported for '%s' instruction.\n", pInstruction);
            sprintf(expectedListOutput, "    :              1 %s\n", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
        }
    }
    
    void testAbsoluteAddressing(const char* pInstruction)
    {
        char               testString[64];
        char               expectedListOutput[128];
        char               expectedErrorOutput[128];

        CHECK_FALSE(m_isZeroPageTreatedAsAbsolute);
        
        sprintf(testString, " %s $100\n", pInstruction);
        runAssembler(testString);
        
        if (!m_isInvalidMode)
        {
            sprintf(expectedListOutput, "0000: %02X 00 01     1 %s\n", m_expectedOpcode, testString);
            validateSuccessfulOutput(expectedListOutput);
        }
        else
        {
            sprintf(expectedErrorOutput, "filename:1: error: Addressing mode of '$100' is not supported for '%s' instruction.\n", pInstruction);
            sprintf(expectedListOutput, "    :              1 %s\n", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
        }
    }
    
    void testZeroPageAddressing(const char* pInstruction)
    {
        char               testString[64];
        char               expectedListOutput[128];
        char               expectedErrorOutput[128];

        sprintf(testString, " %s $ff\n", pInstruction);
        runAssembler(testString);
        
        if (!m_isInvalidMode)
        {
            if (!m_isZeroPageTreatedAsAbsolute)
                sprintf(expectedListOutput, "0000: %02X FF        1 %s\n", m_expectedOpcode, testString);
            else
                sprintf(expectedListOutput, "0000: %02X FF 00     1 %s\n", m_expectedOpcode, testString);
            validateSuccessfulOutput(expectedListOutput);
        }
        else
        {
            sprintf(expectedErrorOutput, "filename:1: error: Addressing mode of '$ff' is not supported for '%s' instruction.\n", pInstruction);
            sprintf(expectedListOutput, "    :              1 %s\n", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
        }
    }
    
    void testImpliedAddressing(const char* pInstruction)
    {
        char               testString[64];
        char               expectedListOutput[128];
        char               expectedErrorOutput[128];

        CHECK_FALSE(m_isZeroPageTreatedAsAbsolute);
        
        sprintf(testString, " %s\n", pInstruction);
        runAssembler(testString);
        if (!m_isInvalidMode)
        {
            sprintf(expectedListOutput, "0000: %02X           1 %s\n", m_expectedOpcode, testString);
            validateSuccessfulOutput(expectedListOutput);
        }
        else
        {
            sprintf(expectedErrorOutput, "filename:1: error: Addressing mode of '(null)' is not supported for '%s' instruction.\n", pInstruction);
            sprintf(expectedListOutput, "    :              1 %s\n", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
        }
    }
    
    void testZeroPageIndexedIndirectAddressing(const char* pInstruction)
    {
        char               testString[64];
        char               expectedListOutput[128];
        char               expectedErrorOutput[128];

        sprintf(testString, " %s ($ff,x)\n", pInstruction);
        runAssembler(testString);
        
        if (!m_isInvalidMode)
        {
            if (!m_isZeroPageTreatedAsAbsolute)
                sprintf(expectedListOutput, "0000: %02X FF        1 %s\n", m_expectedOpcode, testString);
            else
                sprintf(expectedListOutput, "0000: %02X FF 00     1 %s\n", m_expectedOpcode, testString);
            validateSuccessfulOutput(expectedListOutput);
        }
        else
        {
            sprintf(expectedErrorOutput, "filename:1: error: Addressing mode of '($ff,x)' is not supported for '%s' instruction.\n", pInstruction);
            sprintf(expectedListOutput, "    :              1 %s\n", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
        }
    }
    
    void testIndirectIndexedAddressing(const char* pInstruction)
    {
        char               testString[64];
        char               expectedListOutput[128];
        char               expectedErrorOutput[128];

        CHECK_FALSE(m_isZeroPageTreatedAsAbsolute);
        
        sprintf(testString, " %s ($ff),y\n", pInstruction);
        runAssembler(testString);
        
        if (!m_isInvalidMode)
        {
            sprintf(expectedListOutput, "0000: %02X FF        1 %s\n", m_expectedOpcode, testString);
            validateSuccessfulOutput(expectedListOutput);
        }
        else
        {
            sprintf(expectedErrorOutput, "filename:1: error: Addressing mode of '($ff),y' is not supported for '%s' instruction.\n", pInstruction);
            sprintf(expectedListOutput, "    :              1 %s\n", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
        }
    }
    
    void testZeroPageIndexedXAddressing(const char* pInstruction)
    {
        char               testString[64];
        char               expectedListOutput[128];
        char               expectedErrorOutput[128];

        sprintf(testString, " %s $ff,x\n", pInstruction);
        runAssembler(testString);
        
        if (!m_isInvalidMode)
        {
            if (!m_isZeroPageTreatedAsAbsolute)
                sprintf(expectedListOutput, "0000: %02X FF        1 %s\n", m_expectedOpcode, testString);
            else
                sprintf(expectedListOutput, "0000: %02X FF 00     1 %s\n", m_expectedOpcode, testString);
            validateSuccessfulOutput(expectedListOutput);
        }
        else
        {
            sprintf(expectedErrorOutput, "filename:1: error: Addressing mode of '$ff,x' is not supported for '%s' instruction.\n", pInstruction);
            sprintf(expectedListOutput, "    :              1 %s\n", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
        }
    }
    
    void testZeroPageIndexedYAddressing(const char* pInstruction)
    {
        char               testString[64];
        char               expectedListOutput[128];
        char               expectedErrorOutput[128];

        sprintf(testString, " %s $ff,y\n", pInstruction);
        runAssembler(testString);
        
        if (!m_isInvalidMode)
        {
            if (!m_isZeroPageTreatedAsAbsolute)
                sprintf(expectedListOutput, "0000: %02X FF        1 %s\n", m_expectedOpcode, testString);
            else
                sprintf(expectedListOutput, "0000: %02X FF 00     1 %s\n", m_expectedOpcode, testString);
            validateSuccessfulOutput(expectedListOutput);
        }
        else
        {
            sprintf(expectedErrorOutput, "filename:1: error: Addressing mode of '$ff,y' is not supported for '%s' instruction.\n", pInstruction);
            sprintf(expectedListOutput, "    :              1 %s\n", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
        }
    }
    
    void testAbsoluteIndexedXAddressing(const char* pInstruction)
    {
        char               testString[64];
        char               expectedListOutput[128];
        char               expectedErrorOutput[128];

        CHECK_FALSE(m_isZeroPageTreatedAsAbsolute);
        
        sprintf(testString, " %s $100,x\n", pInstruction);
        runAssembler(testString);
        
        if (!m_isInvalidMode)
        {
            sprintf(expectedListOutput, "0000: %02X 00 01     1 %s\n", m_expectedOpcode, testString);
            validateSuccessfulOutput(expectedListOutput);
        }
        else
        {
            sprintf(expectedErrorOutput, "filename:1: error: Addressing mode of '$100,x' is not supported for '%s' instruction.\n", pInstruction);
            sprintf(expectedListOutput, "    :              1 %s\n", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
        }
    }
    
    void testAbsoluteIndexedYAddressing(const char* pInstruction)
    {
        char               testString[64];
        char               expectedListOutput[128];
        char               expectedErrorOutput[128];

        CHECK_FALSE(m_isZeroPageTreatedAsAbsolute);
        
        sprintf(testString, " %s $100,y\n", pInstruction);
        runAssembler(testString);
        
        if (!m_isInvalidMode)
        {
            sprintf(expectedListOutput, "0000: %02X 00 01     1 %s\n", m_expectedOpcode, testString);
            validateSuccessfulOutput(expectedListOutput);
        }
        else
        {
            sprintf(expectedErrorOutput, "filename:1: error: Addressing mode of '$100,y' is not supported for '%s' instruction.\n", pInstruction);
            sprintf(expectedListOutput, "    :              1 %s\n", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
        }
    }
    
    void testRelativeAddressing(const char* pInstruction)
    {
        char               testString[64];
        char               expectedListOutput[128];

        CHECK_FALSE(m_isZeroPageTreatedAsAbsolute);
        if (m_isInvalidMode)
            return;
        
        sprintf(testString, " %s *\n", pInstruction);
        runAssembler(testString);
        sprintf(expectedListOutput, "0000: %02X FE        1 %s\n", m_expectedOpcode, testString);
        validateSuccessfulOutput(expectedListOutput);
    }
    
    void testAbsoluteIndirectAddressing(const char* pInstruction)
    {
        char               testString[64];
        char               expectedListOutput[128];
        char               expectedErrorOutput[128];

        CHECK_FALSE(m_isZeroPageTreatedAsAbsolute);
        
        sprintf(testString, " %s ($100)\n", pInstruction);
        runAssembler(testString);
        
        if (!m_isInvalidMode)
        {
            sprintf(expectedListOutput, "0000: %02X 00 01     1 %s\n", m_expectedOpcode, testString);
            validateSuccessfulOutput(expectedListOutput);
        }
        else
        {
            sprintf(expectedErrorOutput, "filename:1: error: Addressing mode of '($100)' is not supported for '%s' instruction.\n", pInstruction);
            sprintf(expectedListOutput, "    :              1 %s\n", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
        }
    }
    
    void testAbsoluteIndexedIndirectAddressing(const char* pInstruction)
    {
        char               testString[64];
        char               expectedListOutput[128];
        char               expectedErrorOutput[128];

        CHECK_FALSE(m_isZeroPageTreatedAsAbsolute);
        
        sprintf(testString, " %s ($100,x)\n", pInstruction);
        runAssembler(testString);
        
        if (!m_isInvalidMode)
        {
            sprintf(expectedListOutput, "0000: %02X 00 01     1 %s\n", m_expectedOpcode, testString);
            validateSuccessfulOutput(expectedListOutput);
        }
        else
        {
            sprintf(expectedErrorOutput, "filename:1: error: Addressing mode of '($100,x)' is not supported for '%s' instruction.\n", pInstruction);
            sprintf(expectedListOutput, "    :              1 %s\n", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
        }
    }
    
    void testZeroPageIndirectAddressing(const char* pInstruction)
    {
        char               testString[64];
        char               expectedListOutput[128];
        char               expectedErrorOutput[128];

        sprintf(testString, " %s ($ff)\n", pInstruction);
        runAssembler(testString);
        
        if (!m_isInvalidMode)
        {
            if (!m_isZeroPageTreatedAsAbsolute)
                sprintf(expectedListOutput, "0000: %02X FF        1 %s\n", m_expectedOpcode, testString);
            else
                sprintf(expectedListOutput, "0000: %02X FF 00     1 %s\n", m_expectedOpcode, testString);
            validateSuccessfulOutput(expectedListOutput);
        }
        else
        {
            sprintf(expectedErrorOutput, "filename:1: error: Addressing mode of '($ff)' is not supported for '%s' instruction.\n", pInstruction);
            sprintf(expectedListOutput, "    :              1 %s\n", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
        }
    }
    
    void runAssembler(char* pTestString)
    {
        printfSpy_Unhook();
        printfSpy_Hook(512);
        m_pAssembler = Assembler_CreateFromString(pTestString);
        Assembler_Run(m_pAssembler);
        Assembler_Free(m_pAssembler);
        m_pAssembler = NULL;
    }
    
    void validateSuccessfulOutput(const char* pExpectedListOutput)
    {
        STRCMP_EQUAL(pExpectedListOutput, printfSpy_GetLastOutput());
        LONGS_EQUAL(1, printfSpy_GetCallCount());
    }
    
    void validateFailureOutput(const char* pExpectedErrorOutput, const char* pExpectedListOutput)
    {
        STRCMP_EQUAL(pExpectedErrorOutput, printfSpy_GetLastErrorOutput());
        STRCMP_EQUAL(pExpectedListOutput, printfSpy_GetLastOutput());
        LONGS_EQUAL(2, printfSpy_GetCallCount());
    }
    
    void validateLineInfo(const LineInfo*      pLineInfo, 
                          unsigned short       expectedAddress, 
                          size_t               expectedMachineCodeSize,
                          const char*          pExpectedMachineCode)
    {
        LONGS_EQUAL(expectedMachineCodeSize, pLineInfo->machineCodeSize);
        CHECK_TRUE(0 == memcmp(pLineInfo->machineCode, pExpectedMachineCode, expectedMachineCodeSize));
        LONGS_EQUAL(expectedAddress, pLineInfo->address);
    }
};


TEST(Assembler, FailFirstInitAllocation)
{
    MallocFailureInject_FailAllocation(1);
    m_pAssembler = Assembler_CreateFromString(dupe(""));
    validateOutOfMemoryExceptionThrown();
}

TEST(Assembler, FailSecondInitAllocation)
{
    MallocFailureInject_FailAllocation(2);
    m_pAssembler = Assembler_CreateFromString(dupe(""));
    validateOutOfMemoryExceptionThrown();
}

TEST(Assembler, FailThirdInitAllocation)
{
    MallocFailureInject_FailAllocation(3);
    m_pAssembler = Assembler_CreateFromString(dupe(""));
    validateOutOfMemoryExceptionThrown();
}

TEST(Assembler, FailFourthInitAllocation)
{
    MallocFailureInject_FailAllocation(4);
    m_pAssembler = Assembler_CreateFromString(dupe(""));
    validateOutOfMemoryExceptionThrown();
}

TEST(Assembler, FailFifthInitAllocation)
{
    MallocFailureInject_FailAllocation(5);
    m_pAssembler = Assembler_CreateFromString(dupe(""));
    validateOutOfMemoryExceptionThrown();
}

TEST(Assembler, EmptyString)
{
    m_pAssembler = Assembler_CreateFromString(dupe(""));
    CHECK(m_pAssembler != NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(0, printfSpy_GetCallCount());
}

TEST(Assembler, InitFromEmptyFile)
{
    createSourceFile("");
    m_pAssembler = Assembler_CreateFromFile(g_sourceFilename);
    CHECK(m_pAssembler != NULL);
}

TEST(Assembler, InitFromNonExistantFile)
{
    m_pAssembler = Assembler_CreateFromFile("foo.noexist.bar");
    validateFileNotFoundExceptionThrown();
}

TEST(Assembler, FailFirstAllocationDuringFileInit)
{
    createSourceFile(" ORG $800\r\n");
    MallocFailureInject_FailAllocation(1);
    m_pAssembler = Assembler_CreateFromFile(g_sourceFilename);
    validateOutOfMemoryExceptionThrown();
}

TEST(Assembler, FailSecondAllocationDuringFileInit)
{
    createSourceFile(" ORG $800\r\n");
    MallocFailureInject_FailAllocation(2);
    m_pAssembler = Assembler_CreateFromFile(g_sourceFilename);
    validateOutOfMemoryExceptionThrown();
}

TEST(Assembler, FailThirdAllocationDuringFileInit)
{
    createSourceFile(" ORG $800\r\n");
    MallocFailureInject_FailAllocation(3);
    m_pAssembler = Assembler_CreateFromFile(g_sourceFilename);
    validateOutOfMemoryExceptionThrown();
}

TEST(Assembler, FailFourthAllocationDuringFileInit)
{
    createSourceFile(" ORG $800\r\n");
    MallocFailureInject_FailAllocation(4);
    m_pAssembler = Assembler_CreateFromFile(g_sourceFilename);
    validateOutOfMemoryExceptionThrown();
}

TEST(Assembler, FailFifthAllocationDuringFileInit)
{
    createSourceFile(" ORG $800\r\n");
    MallocFailureInject_FailAllocation(5);
    m_pAssembler = Assembler_CreateFromFile(g_sourceFilename);
    validateOutOfMemoryExceptionThrown();
}

TEST(Assembler, FailSixthAllocationDuringFileInit)
{
    createSourceFile(" ORG $800\r\n");
    MallocFailureInject_FailAllocation(6);
    m_pAssembler = Assembler_CreateFromFile(g_sourceFilename);
    validateOutOfMemoryExceptionThrown();
}

TEST(Assembler, InitAndRunFromShortFile)
{
    createSourceFile("* Symbols\n"
                     "SYM1 = $1\n"
                     "SYM2 EQU $2\n");
    m_pAssembler = Assembler_CreateFromFile(g_sourceFilename);
    CHECK(m_pAssembler != NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(3, printfSpy_GetCallCount());
}

TEST(Assembler, FailAllocationOnLongLine)
{
    char longLine[257];
    
    memset(longLine, ' ', sizeof(longLine));
    longLine[ARRAYSIZE(longLine)-1] = '\0';
    m_pAssembler = Assembler_CreateFromString(longLine);
    CHECK(m_pAssembler != NULL);

    MallocFailureInject_FailAllocation(1);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(0, printfSpy_GetCallCount());
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}

TEST(Assembler, FailAllocationOnLineInfoAllocation)
{
    m_pAssembler = Assembler_CreateFromString(dupe("* Comment Line."));
    CHECK(m_pAssembler != NULL);

    MallocFailureInject_FailAllocation(1);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(0, printfSpy_GetCallCount());
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}

TEST(Assembler, RunOnLongLine)
{
    char longLine[257];
    
    memset(longLine, ' ', sizeof(longLine));
    longLine[ARRAYSIZE(longLine)-1] = '\0';
    m_pAssembler = Assembler_CreateFromString(longLine);
    CHECK(m_pAssembler != NULL);

    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(1, printfSpy_GetCallCount());
    LONGS_EQUAL(0, Assembler_GetErrorCount(m_pAssembler) );
}

TEST(Assembler, LabelTooLong)
{
    char longLine[257];
    
    memset(longLine, 'a', sizeof(longLine));
    longLine[ARRAYSIZE(longLine)-1] = '\0';
    m_pAssembler = Assembler_CreateFromString(longLine);
    CHECK(m_pAssembler != NULL);

    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    CHECK(NULL != strstr(printfSpy_GetPreviousOutput(), " label is too long.") );
}

TEST(Assembler, LocalLabelTooLong)
{
    char longLine[255+1+2+1];
    
    memset(longLine, 'a', 255);
    strcpy(&longLine[sizeof(longLine) - 4], "\n:b");
    m_pAssembler = Assembler_CreateFromString(longLine);
    CHECK(m_pAssembler != NULL);

    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(3, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    CHECK(NULL != strstr(printfSpy_GetLastErrorOutput(), " label is too long.") );
}

TEST(Assembler, SpecifySameLabelTwice)
{
    m_pAssembler = Assembler_CreateFromString(dupe("entry lda #$60\n"
                                                   "entry lda #$61\n"));
    runAssemblerAndValidateFailure("filename:2: error: 'entry' symbol has already been defined.\n",
                                   "0002: A9 61        2 entry lda #$61\n", 3);
}

TEST(Assembler, LocalLabelDefineBeforeGlobalLabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe(":local_label\n"));
    CHECK(m_pAssembler != NULL);

    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: ':local_label' local label isn't allowed before first global label.\n", printfSpy_GetPreviousOutput());
}

TEST(Assembler, LocalLabelReferenceBeforeGlobalLabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta :local_label\n"));
    CHECK(m_pAssembler != NULL);

    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: ':local_label' local label isn't allowed before first global label.\n", printfSpy_GetPreviousOutput());
}

TEST(Assembler, LabelStartsWithInvalidCharacter)
{
    m_pAssembler = Assembler_CreateFromString(dupe("9Label sta $23\n"));
    CHECK(m_pAssembler != NULL);

    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: '9Label' label starts with invalid character.\n", printfSpy_GetPreviousOutput());
}

TEST(Assembler, LabelContainsInvalidCharacter)
{
    m_pAssembler = Assembler_CreateFromString(dupe("Label. sta $23\n"));
    CHECK(m_pAssembler != NULL);

    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: 'Label.' label contains invalid character, '.'.\n", printfSpy_GetPreviousOutput());
}

TEST(Assembler, ForwardReferenceLabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " sta label\n"
                                                   "label sta $2b\n"));
    CHECK(m_pAssembler != NULL);
    runAssemblerAndValidateOutputIsTwoLinesOf("0800: 8D 03 08     2  sta label\n",
                                              "0803: 85 2B        3 label sta $2b\n", 3);
}

TEST(Assembler, FailLineInfoAllocationDuringForwardReferenceLabelFixup)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " sta label\n"
                                                   "label sta $2b\n"));
    CHECK(m_pAssembler != NULL);

    MallocFailureInject_FailAllocation(8);
    runAssemblerAndValidateFailure("filename:3: error: Failed to allocate space for updating forward references to 'label' symbol.\n",
                                   "0803: 85 2B        3 label sta $2b\n", 4);
    MallocFailureInject_Restore();
}

TEST(Assembler, FailToDefineLabelTwiceAfterForwardReferenced)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " sta label\n"
                                                   "label sta $2b\n"
                                                   "label sta $2c\n"));
    CHECK(m_pAssembler != NULL);
    runAssemblerAndValidateFailure("filename:4: error: 'label' symbol has already been defined.\n",
                                   "0805: 85 2C        4 label sta $2c\n", 5);
}

TEST(Assembler, LocalLabelPlusOffsetBackwardReference)
{
    m_pAssembler = Assembler_CreateFromString(dupe("func1 sta $20\n"
                                                   ":local sta $20\n"
                                                   "func2 sta $21\n"
                                                   ":local sta $22\n"
                                                   " sta :local+1\n"));
    runAssemblerAndValidateLastLineIs("0008: 85 07        5  sta :local+1\n", 5);
}

TEST(Assembler, LocalLabelPlusOffsetForwardReference)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   "func1 sta $20\n"
                                                   ":local sta $20\n"
                                                   "func2 sta $21\n"
                                                   " sta :local+1\n"
                                                   ":local sta $22\n"));
    
    runAssemblerAndValidateOutputIsTwoLinesOf("0806: 8D 0A 08     5  sta :local+1\n",
                                              "0809: 85 22        6 :local sta $22\n", 6);
}

TEST(Assembler, GlobalLabelPlusOffsetForwardReference)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " sta globalLabel+1\n"
                                                   "globalLabel sta $22\n"));
    
    runAssemblerAndValidateOutputIsTwoLinesOf("0800: 8D 04 08     2  sta globalLabel+1\n",
                                              "0803: 85 22        3 globalLabel sta $22\n", 3);
}

TEST(Assembler, CascadedForwardReferenceOfEQUToLabel)
{
    LineInfo* pSecondLine;
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " sta 1+equLabel\n"
                                                   "equLabel equ lineLabel\n"
                                                   "lineLabel sta $22\n"));
    Assembler_Run(m_pAssembler);
    pSecondLine = m_pAssembler->linesHead.pNext->pNext;
    LONGS_EQUAL(3, pSecondLine->machineCodeSize);
    CHECK(0 == memcmp(pSecondLine->machineCode, "\x8d\x04\x08", 3));
}

TEST(Assembler, MultipleForwardReferencesToSameLabel)
{
    LineInfo* pSecondLine;
    LineInfo* pThirdLine;
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " sta 1+label\n"
                                                   " sta label+1\n"
                                                   "label sta $22\n"));
    Assembler_Run(m_pAssembler);
    pSecondLine = m_pAssembler->linesHead.pNext->pNext;
    LONGS_EQUAL(3, pSecondLine->machineCodeSize);
    CHECK(0 == memcmp(pSecondLine->machineCode, "\x8d\x07\x08", 3));
    pThirdLine = pSecondLine->pNext;
    LONGS_EQUAL(3, pSecondLine->machineCodeSize);
    CHECK(0 == memcmp(pSecondLine->machineCode, "\x8d\x07\x08", 3));
}

TEST(Assembler, FailZeroPageForwardReference)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta globalLabel\n"
                                                   "globalLabel sta $22\n"));
    
    runAssemblerAndValidateFailure("filename:1: error: Couldn't properly infer size of 'globalLabel' forward reference.\n",
                                   "0003: 85 22        2 globalLabel sta $22\n", 3);
}

TEST(Assembler, ReferenceNonExistantLabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta badLabel\n"));
    runAssemblerAndValidateFailure("filename:1: error: The 'badLabel' label is undefined.\n",
                                   "0000: 8D 00 00     1  sta badLabel\n", 2);
}

TEST(Assembler, EQULabelStartsWithInvalidCharacter)
{
    m_pAssembler = Assembler_CreateFromString(dupe("9Label EQU $23\n"));
    CHECK(m_pAssembler != NULL);

    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: '9Label' label starts with invalid character.\n", printfSpy_GetPreviousOutput());
}

TEST(Assembler, EQULabelContainsInvalidCharacter)
{
    m_pAssembler = Assembler_CreateFromString(dupe("Label. EQU $23\n"));
    CHECK(m_pAssembler != NULL);

    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: 'Label.' label contains invalid character, '.'.\n", printfSpy_GetPreviousOutput());
}

TEST(Assembler, EQULabelIsLocal)
{
    m_pAssembler = Assembler_CreateFromString(dupe(":Label EQU $23\n"));
    CHECK(m_pAssembler != NULL);

    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: ':Label' can't be a local label when used with EQU.\n", printfSpy_GetPreviousOutput());
}

TEST(Assembler, ForwardReferenceEQULabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta label\n"
                                                   "label equ $ffff\n"));
    CHECK(m_pAssembler != NULL);
    runAssemblerAndValidateOutputIsTwoLinesOf("0000: 8D FF FF     1  sta label\n",
                                              "    :    =FFFF     2 label equ $ffff\n");
}

TEST(Assembler, FailToDefineEquLabelTwiceAfterForwardReferenced)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta label\n"
                                                   "label equ $ff2b\n"
                                                   "label equ $ff2c\n"));
    CHECK(m_pAssembler != NULL);
    runAssemblerAndValidateFailure("filename:3: error: 'label' symbol has already been defined.\n",
                                   "    :              3 label equ $ff2c\n", 4);
}

TEST(Assembler, CommentLine)
{
    m_pAssembler = Assembler_CreateFromString(dupe("*  boot\n"));
    runAssemblerAndValidateOutputIs("    :              1 *  boot\n");
}

TEST(Assembler, InvalidOperatorFromStringSource)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" foo bar\n"));
    runAssemblerAndValidateFailure("filename:1: error: 'foo' is not a recognized mnemonic or macro.\n", 
                                   "    :              1  foo bar\n");
}

TEST(Assembler, InvalidOperatorFromFileSource)
{
    createSourceFile(" foo bar\n");
    m_pAssembler = Assembler_CreateFromFile(g_sourceFilename);
    runAssemblerAndValidateFailure("AssemblerTest.S:1: error: 'foo' is not a recognized mnemonic or macro.\n", 
                                   "    :              1  foo bar\n");
}

TEST(Assembler, EqualSignDirective)
{
    m_pAssembler = Assembler_CreateFromString(dupe("org = $800\n"));
    runAssemblerAndValidateOutputIs("    :    =0800     1 org = $800\n");
}

TEST(Assembler, EQUDirective)
{
    m_pAssembler = Assembler_CreateFromString(dupe("org EQU $800\n"));
    runAssemblerAndValidateOutputIs("    :    =0800     1 org EQU $800\n");
}

TEST(Assembler, MultipleDefinedSymbolFailure)
{
    m_pAssembler = Assembler_CreateFromString(dupe("org = $800\n"
                                                   "org EQU $900\n"));
    runAssemblerAndValidateFailure("filename:2: error: 'org' symbol has already been defined.\n", 
                                   "    :              2 org EQU $900\n", 3);
}

TEST(Assembler, FailAllocationDuringSymbolCreation)
{
    m_pAssembler = Assembler_CreateFromString(dupe("org = $800\n"));
    MallocFailureInject_FailAllocation(2);
    runAssemblerAndValidateFailure("filename:1: error: Failed to allocate space for 'org' symbol.\n", 
                                   "    :              1 org = $800\n");
}

TEST(Assembler, InvalidExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe("org EQU (800\n"));
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '(800' expression.\n", 
                                   "    :              1 org EQU (800\n");
}

TEST(Assembler, IgnoreLSTDirective)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lst off\n"));
    runAssemblerAndValidateOutputIs("    :              1  lst off\n");
}

TEST(Assembler, HEXDirectiveWithSingleValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 01\n"));
    runAssemblerAndValidateOutputIs("0000: 01           1  hex 01\n");
}

TEST(Assembler, HEXDirectiveWithThreeValuesAndCommas)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 0e,0c,0a\n"));
    runAssemblerAndValidateOutputIs("0000: 0E 0C 0A     1  hex 0e,0c,0a\n");
}

TEST(Assembler, HEXDirectiveWithThreeValues)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 0e0c0a\n"));
    runAssemblerAndValidateOutputIs("0000: 0E 0C 0A     1  hex 0e0c0a\n");
}

TEST(Assembler, HEXDirectiveWithFourValues)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 01,02,03,04\n"));
    runAssemblerAndValidateOutputIsTwoLinesOf("0000: 01 02 03     1  hex 01,02,03,04\n",
                                              "0003: 04      \n");
}

TEST(Assembler, HEXDirectiveWithSixValues)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 01,02,03,04,05,06\n"));
    runAssemblerAndValidateOutputIsTwoLinesOf("0000: 01 02 03     1  hex 01,02,03,04,05,06\n",
                                              "0003: 04 05 06\n");
}

TEST(Assembler, HEXDirectiveWithMaximumOf32Value)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20\n"));
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(0, Assembler_GetErrorCount(m_pAssembler) );
    LONGS_EQUAL(11, printfSpy_GetCallCount());
    STRCMP_EQUAL("001B: 1C 1D 1E\n", printfSpy_GetPreviousOutput());
    STRCMP_EQUAL("001E: 1F 20   \n", printfSpy_GetLastOutput());
}

TEST(Assembler, HEXDirectiveWith33Values_1MoreThanSupported)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f2021\n"));
    runAssemblerAndValidateFailure("filename:1: error: '0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f2021' contains more than 32 values.\n", 
                                   "    :              1  hex 0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f2021\n");
}

TEST(Assembler, HEXDirectiveOnTwoLines)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 01\n"
                                                   " hex 02\n"));
    runAssemblerAndValidateOutputIsTwoLinesOf("0000: 01           1  hex 01\n",
                                              "0001: 02           2  hex 02\n");
}

TEST(Assembler, HEXDirectiveWithUpperCaseHex)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex FA\n"));
    runAssemblerAndValidateOutputIs("0000: FA           1  hex FA\n");
}

TEST(Assembler, HEXDirectiveWithLowerCaseHex)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex fa\n"));
    runAssemblerAndValidateOutputIs("0000: FA           1  hex fa\n");
}

TEST(Assembler, HEXDirectiveWithOddDigitCount)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex fa0\n"));
    runAssemblerAndValidateFailure("filename:1: error: 'fa0' doesn't contain an even number of hex digits.\n",
                                   "    :              1  hex fa0\n");
}

TEST(Assembler, HEXDirectiveWithInvalidDigit)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex fg\n"));
    runAssemblerAndValidateFailure("filename:1: error: 'fg' contains an invalid hex digit.\n",
                                   "    :              1  hex fg\n");
}

TEST(Assembler, ORGDirectiveWithLiteralValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $900\n"
                                                   " hex 01\n"));
    runAssemblerAndValidateOutputIsTwoLinesOf("    :              1  org $900\n", 
                                              "0900: 01           2  hex 01\n");
}

TEST(Assembler, ORGDirectiveWithInvalidExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org +900\n"));
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '+900' expression.\n",
                                   "    :              1  org +900\n");
}

TEST(Assembler, ORGDirectiveWithSymbolValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe("org = $800\n"
                                                   " org org\n"
                                                   " hex 01\n"));
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(3, printfSpy_GetCallCount());
    STRCMP_EQUAL("    :              2  org org\n", printfSpy_GetPreviousOutput());
    STRCMP_EQUAL("0800: 01           3  hex 01\n", printfSpy_GetLastOutput());
    LONGS_EQUAL(0, Assembler_GetErrorCount(m_pAssembler) );
}

TEST(Assembler, ORGDirectiveWithInvalidImmediate)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org #$00\n"));
    runAssemblerAndValidateFailure("filename:1: error: '#$00' doesn't specify an absolute address.\n",
                                   "    :              1  org #$00\n");
}

TEST(Assembler, DUMandDEND_Directive)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " dum $00\n"
                                                   " hex ff\n"
                                                   " dend\n"
                                                   " hex fe\n"));
    Assembler_Run(m_pAssembler);
    
    LineInfo* pThirdLine = m_pAssembler->linesHead.pNext->pNext->pNext;
    LineInfo* pFifthLine = pThirdLine->pNext->pNext;
    
    validateLineInfo(pThirdLine, 0x0000, 1, "\xff");
    validateLineInfo(pFifthLine, 0x0800, 1, "\xfe");
}

TEST(Assembler, TwoDUMandDEND_Directive)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " dum $00\n"
                                                   " hex ff\n"
                                                   " dum $100\n"
                                                   " hex fe\n"
                                                   " dend\n"
                                                   " hex fd\n"));
    Assembler_Run(m_pAssembler);
    
    LineInfo* pThirdLine = m_pAssembler->linesHead.pNext->pNext->pNext;
    LineInfo* pFifthLine = pThirdLine->pNext->pNext;
    LineInfo* pSeventhLine = pFifthLine->pNext->pNext;
    
    validateLineInfo(pThirdLine, 0x0000, 1, "\xff");
    validateLineInfo(pFifthLine, 0x0100, 1, "\xfe");
    validateLineInfo(pSeventhLine, 0x0800, 1, "\xfd");
}

TEST(Assembler, DUM_ORG_andDEND_Directive)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " dum $00\n"
                                                   " hex ff\n"
                                                   " org $100\n"
                                                   " hex fe\n"
                                                   " dend\n"
                                                   " hex fd\n"));
    Assembler_Run(m_pAssembler);
    
    LineInfo* pThirdLine = m_pAssembler->linesHead.pNext->pNext->pNext;
    LineInfo* pFifthLine = pThirdLine->pNext->pNext;
    LineInfo* pSeventhLine = pFifthLine->pNext->pNext;
    
    validateLineInfo(pThirdLine, 0x0000, 1, "\xff");
    validateLineInfo(pFifthLine, 0x0100, 1, "\xfe");
    validateLineInfo(pSeventhLine, 0x0800, 1, "\xfd");
}

TEST(Assembler, DUMDirectiveWithInvalidImmediateExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dum #$00\n"));
    runAssemblerAndValidateFailure("filename:1: error: '#$00' doesn't specify an absolute address.\n",
                                   "    :              1  dum #$00\n");
}

TEST(Assembler, DEND_DirectiveWithoutDUM)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dend\n"));
    runAssemblerAndValidateFailure("filename:1: error: dend isn't allowed without a preceding DUM directive.\n",
                                   "    :              1  dend\n");
}

TEST(Assembler, DS_DirectiveWithSmallRepeatValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds 1\n"
                                                   " hex ff\n"));
    runAssemblerAndValidateOutputIsTwoLinesOf("0000: 00           1  ds 1\n", 
                                              "0001: FF           2  hex ff\n");
}

TEST(Assembler, DS_DirectiveWithRepeatValueGreaterThan32)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds 42\n"
                                                   " hex ff\n"));
    runAssemblerAndValidateOutputIsTwoLinesOf("0027: 00 00 00\n", 
                                              "002A: FF           2  hex ff\n", 15);
}

TEST(Assembler, DS_DirectiveWithBackSlashUnsupported)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds \\\n"));
    runAssemblerAndValidateFailure("filename:1: error: The '\\' label is undefined.\n",
                                   "    :              1  ds \\\n");
}

TEST(Assembler, DS_DirectiveWithSecondExpressionToSpecifyFillValueUnsupported)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds 2,$ff\n"));
    runAssemblerAndValidateFailure("filename:1: error: ',' is unexpected operator.\n",
                                   "    :              1  ds 2,$ff\n");
}

TEST(Assembler, FailWithInvalidImmediateValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lda #$100\n"));
    runAssemblerAndValidateFailure("filename:1: error: Immediate expression '$100' doesn't fit in 8-bits.\n",
                                   "    :              1  lda #$100\n");
}

TEST(Assembler, FailWithInvalidExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta +ff\n"));
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '+ff' expression.\n",
                                   "    :              1  sta +ff\n");
}

TEST(Assembler, STAAbsoluteViaLabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   "entry lda #$60\n"
                                                   " sta entry\n"));
    runAssemblerAndValidateLastLineIs("0802: 8D 00 08     3  sta entry\n", 3);
}

TEST(Assembler, STAZeroPageAbsoluteViaLabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe("entry lda #$60\n"
                                                   " sta entry\n"));
    runAssemblerAndValidateLastLineIs("0002: 85 00        2  sta entry\n", 2);
}

TEST(Assembler, BCS_ValidAddressingMode)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" bcs *+129\n"));
    runAssemblerAndValidateOutputIs("0000: B0 7F        1  bcs *+129\n");
}

TEST(Assembler, BCS_InvalidAddressingModes)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" bcs #1\n"
                                                   " bcs\n"
                                                   " bcs ($ff,x)\n"
                                                   " bcs ($ff),y\n"
                                                   " bcs $ff,x\n"
                                                   " bcs $ff,y\n"
                                                   " bcs $100,x\n"
                                                   " bcs $100,y\n"
                                                   " bcs ($100)\n"
                                                   " bcs ($100,x)\n"
                                                   " bcs ($ff)\n"));
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(11, Assembler_GetErrorCount(m_pAssembler));
}

TEST(Assembler, BEQ_ZeroPageMaxNegativeTarget)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $0090\n"
                                                   " beq *-126\n"));
    runAssemblerAndValidateLastLineIs("0090: F0 80        2  beq *-126\n", 2);
}

TEST(Assembler, BEQ_ZeroPageMaxPositiveTarget)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $0000\n"
                                                   " beq *+129\n"));
    runAssemblerAndValidateLastLineIs("0000: F0 7F        2  beq *+129\n", 2);
}

TEST(Assembler, BEQ_ZeroPageInvalidNegativeTarget)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $0090\n"
                                                   " beq *-127\n"));
    runAssemblerAndValidateFailure("filename:2: error: Relative offset of '*-127' exceeds the allowed -128 to 127 range.\n",
                                   "    :              2  beq *-127\n", 3);
}

TEST(Assembler, BEQ_ZeroPageInvalidPositiveTarget)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $0000\n"
                                                   " beq *+130\n"));
    runAssemblerAndValidateFailure("filename:2: error: Relative offset of '*+130' exceeds the allowed -128 to 127 range.\n",
                                   "    :              2  beq *+130\n", 3);
}

TEST(Assembler, BEQ_AbsoluteTarget)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $0800\n"
                                                   " beq *+2\n"));
    runAssemblerAndValidateLastLineIs("0800: F0 00        2  beq *+2\n", 2);
}

TEST(Assembler, BEQ_ForwardLabelReference)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $0800\n"
                                                   " beq label\n"
                                                   "label\n"));
    runAssemblerAndValidateOutputIsTwoLinesOf("0800: F0 00        2  beq label\n",
                                              "    :              3 label\n", 3);
}

TEST(Assembler, BEQ_InvalidAddressingModes)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" beq #1\n"
                                                   " beq\n"
                                                   " beq ($ff,x)\n"
                                                   " beq ($ff),y\n"
                                                   " beq $ff,x\n"
                                                   " beq $ff,y\n"
                                                   " beq $100,x\n"
                                                   " beq $100,y\n"
                                                   " beq ($100)\n"
                                                   " beq ($100,x)\n"
                                                   " beq ($ff)\n"));
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(11, Assembler_GetErrorCount(m_pAssembler));
}

TEST(Assembler, BMI_ValidAddressingMode)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" bmi *+129\n"));
    runAssemblerAndValidateOutputIs("0000: 30 7F        1  bmi *+129\n");
}

TEST(Assembler, BMI_InvalidAddressingModes)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" bmi #1\n"
                                                   " bmi\n"
                                                   " bmi ($ff,x)\n"
                                                   " bmi ($ff),y\n"
                                                   " bmi $ff,x\n"
                                                   " bmi $ff,y\n"
                                                   " bmi $100,x\n"
                                                   " bmi $100,y\n"
                                                   " bmi ($100)\n"
                                                   " bmi ($100,x)\n"
                                                   " bmi ($ff)\n"));
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(11, Assembler_GetErrorCount(m_pAssembler));
}

TEST(Assembler, BNE_ValidAddressingMode)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" bne *+129\n"));
    runAssemblerAndValidateOutputIs("0000: D0 7F        1  bne *+129\n");
}

TEST(Assembler, BNE_InvalidAddressingModes)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" bne #1\n"
                                                   " bne\n"
                                                   " bne ($ff,x)\n"
                                                   " bne ($ff),y\n"
                                                   " bne $ff,x\n"
                                                   " bne $ff,y\n"
                                                   " bne $100,x\n"
                                                   " bne $100,y\n"
                                                   " bne ($100)\n"
                                                   " bne ($100,x)\n"
                                                   " bne ($ff)\n"));
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(11, Assembler_GetErrorCount(m_pAssembler));
}

TEST(Assembler, BPL_ValidAddressingMode)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" bpl *+129\n"));
    runAssemblerAndValidateOutputIs("0000: 10 7F        1  bpl *+129\n");
}

TEST(Assembler, BPL_InvalidAddressingModes)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" bpl #1\n"
                                                   " bpl\n"
                                                   " bpl ($ff,x)\n"
                                                   " bpl ($ff),y\n"
                                                   " bpl $ff,x\n"
                                                   " bpl $ff,y\n"
                                                   " bpl $100,x\n"
                                                   " bpl $100,y\n"
                                                   " bpl ($100)\n"
                                                   " bpl ($100,x)\n"
                                                   " bpl ($ff)\n"));
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(11, Assembler_GetErrorCount(m_pAssembler));
}

TEST(Assembler, ASL_TableDrivenTest)
{
    test6502Instruction("asl", "XX,0E,06,0A,XX,XX,16,XX,1E,XX,XX,XX,XX,XX");
}

TEST(Assembler, BIT_TableDrivenTest)
{
    test6502Instruction("bit", "89,2C,24,XX,XX,XX,34,XX,3C,XX,XX,XX,XX,XX");
}

TEST(Assembler, CLC_TableDrivenTest)
{
    test6502Instruction("clc", "XX,XX,XX,18,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(Assembler, CMP_TableDrivenTest)
{
    test6502Instruction("cmp", "C9,CD,C5,XX,C1,D1,D5,^D9,DD,D9,XX,XX,XX,D2");
}

TEST(Assembler, DEC_TableDrivenTest)
{
    test6502Instruction("dec", "XX,CE,C6,XX,XX,XX,D6,XX,DE,XX,XX,XX,XX,XX");
}

TEST(Assembler, DEX_TableDrivenTest)
{
    test6502Instruction("dex", "XX,XX,XX,CA,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(Assembler, INC_TableDrivenTest)
{
    test6502Instruction("inc", "XX,EE,E6,XX,XX,XX,F6,XX,FE,XX,XX,XX,XX,XX");
}

TEST(Assembler, INY_TableDrivenTest)
{
    test6502Instruction("iny", "XX,XX,XX,C8,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(Assembler, JMP_TableDrivenTest)
{
    test6502Instruction("jmp", "XX,4C,^4C,XX,^7C,XX,XX,XX,XX,XX,XX,6C,7C,^6C");
}

TEST(Assembler, JSR_TableDrivenTest)
{
    test6502Instruction("jsr", "XX,20,^20,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(Assembler, LDA_TableDrivenTest)
{
    test6502Instruction("lda", "A9,AD,A5,XX,A1,B1,B5,^B9,BD,B9,XX,XX,XX,B2");
}

TEST(Assembler, LDX_TableDrivenTest)
{
    test6502Instruction("ldx", "A2,AE,A6,XX,XX,XX,XX,B6,XX,BE,XX,XX,XX,XX");
}

TEST(Assembler, LDY_TableDrivenTest)
{
    test6502Instruction("ldy", "A0,AC,A4,XX,XX,XX,B4,XX,BC,XX,XX,XX,XX,XX");
}

TEST(Assembler, LSR_TableDrivenTest)
{
    test6502Instruction("lsr", "XX,4E,46,4A,XX,XX,56,XX,5E,XX,XX,XX,XX,XX");
}

TEST(Assembler, ORA_TableDrivenTest)
{
    test6502Instruction("ora", "09,0D,05,XX,01,11,15,^19,1D,19,XX,XX,XX,12");
}

TEST(Assembler, RTS_TableDrivenTest)
{
    test6502Instruction("rts", "XX,XX,XX,60,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(Assembler, STA_TableDrivenTest)
{
    test6502Instruction("sta", "XX,8D,85,XX,81,91,95,^99,9D,99,XX,XX,XX,92");
}

TEST(Assembler, STX_TableDrivenTest)
{
    test6502Instruction("stx", "XX,8E,86,XX,XX,XX,XX,96,XX,XX,XX,XX,XX,XX");
}

TEST(Assembler, STY_TableDrivenTest)
{
    test6502Instruction("sty", "XX,8C,84,XX,XX,XX,94,XX,XX,XX,XX,XX,XX,XX");
}

TEST(Assembler, TXA_TableDrivenTest)
{
    test6502Instruction("txa", "XX,XX,XX,8A,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}
