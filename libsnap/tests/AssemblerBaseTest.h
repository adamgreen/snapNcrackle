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
    Assembler*      m_pAssembler;
    const char*     m_argv[10];
    FILE*           m_pFile;
    char*           m_pReadBuffer;
    SnapCommandLine m_commandLine;
    int             m_argc;
    int             m_isInvalidMode;
    int             m_isZeroPageTreatedAsAbsolute;
    int             m_isInvalidInstruction;
    int             m_switchTo65c02;
    char            m_buffer[128];
    unsigned char   m_expectedOpcode;
    
    void setup()
    {
        clearExceptionCode();
        printfSpy_Hook(512);
        m_pAssembler = NULL;
        m_pFile = NULL;
        m_pReadBuffer = NULL;
        m_isInvalidInstruction = FALSE;
        m_switchTo65c02 = FALSE;
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
    
    void test6502RelativeBranchInstruction(const char* pInstruction, unsigned char opcode)
    {
        char               testString[64];
        char               expectedListOutput[128];
        char               expectedErrorOutput[128];

        sprintf(testString, " %s *+129\n", pInstruction);
        runAssembler(testString);
        if (m_isInvalidInstruction)
        {
            sprintf(expectedErrorOutput, "filename:1: error: '%s' is not a recognized mnemonic or macro.\n", pInstruction);
            sprintf(expectedListOutput, "    :              1 %s\n", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
            return;
        }
        else
        {
            sprintf(expectedListOutput, "8000: %02X 7F        1 %s\n", opcode, testString);
            validateSuccessfulOutput(expectedListOutput);
        }
        
        m_isInvalidMode = TRUE;
        m_expectedOpcode = 0x00;
        m_isZeroPageTreatedAsAbsolute = FALSE;
        testImmediateAddressing(pInstruction);
        testImpliedAddressing(pInstruction);
        testZeroPageIndexedIndirectAddressing(pInstruction);
        testIndirectIndexedAddressing(pInstruction);
        testZeroPageIndexedXAddressing(pInstruction);
        testZeroPageIndexedYAddressing(pInstruction);
        testAbsoluteIndexedXAddressing(pInstruction);
        testAbsoluteIndexedYAddressing(pInstruction);
        testRelativeAddressing(pInstruction);
        testAbsoluteIndirectAddressing(pInstruction);
        testAbsoluteIndexedIndirectAddressing(pInstruction);
        testZeroPageIndirectAddressing(pInstruction);
    }
    
    void test65c02OnlyInstruction(const char* pInstruction, const char* pInstructionTableEntry)
    {
        m_isInvalidInstruction = TRUE;
        m_switchTo65c02 = FALSE;
        testInstruction(pInstruction, pInstructionTableEntry);

        m_isInvalidInstruction = FALSE;
        m_switchTo65c02 = TRUE;
        testInstruction(pInstruction, pInstructionTableEntry);
    }

    void test6502_65c02Instruction(const char* pInstruction, const char* pInstructionTableEntry)
    {
        m_switchTo65c02 = FALSE;
        testInstruction(pInstruction, pInstructionTableEntry);

        m_switchTo65c02 = TRUE;
        testInstruction(pInstruction, pInstructionTableEntry);
    }

    void testInstruction(const char* pInstruction, const char* pInstructionTableEntry)
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
            if (*pCurr == '*')
            {
                m_isInvalidMode = m_switchTo65c02 ? FALSE : TRUE;
                pCurr++;
            }

            m_expectedOpcode = strtoul(pCurr, NULL, 16);
        }
            
        if (pCurr[2] == ',')
            return pCurr + 3;
        else
            return pCurr + 2;
    }
    
    typedef struct AddressingModeStrings
    {
        const char* pTestOperand;
        const char* pSuccessfulOperandMachineCode;
        const char* pSuccessfulAbsoluteOperandMachineCode;
    } AddressingModeStrings;
    
    void testAddressingMode(const char* pInstruction, const AddressingModeStrings* pAddressingModeStrings)
    {
        char testString[64];
        char expectedListOutput[128];
        char expectedErrorOutput[128];

        assert ( pAddressingModeStrings->pSuccessfulAbsoluteOperandMachineCode || !m_isZeroPageTreatedAsAbsolute );
        
        sprintf(testString, " %s %s\n", pInstruction, pAddressingModeStrings->pTestOperand);
        runAssembler(testString);
        if (m_isInvalidInstruction)
        {
            sprintf(expectedErrorOutput, 
                    "filename:1: error: '%s' is not a recognized mnemonic or macro.\n", 
                    pInstruction);
            sprintf(expectedListOutput, "    :              1 %s\n", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
        }
        else if (!m_isInvalidMode)
        {
            sprintf(expectedListOutput, 
                    "8000: %02X %s     1 %s\n", 
                    m_expectedOpcode, 
                    m_isZeroPageTreatedAsAbsolute ? pAddressingModeStrings->pSuccessfulAbsoluteOperandMachineCode :
                                                    pAddressingModeStrings->pSuccessfulOperandMachineCode, 
                    testString);
            validateSuccessfulOutput(expectedListOutput);
        } 
        else
        {
            sprintf(expectedErrorOutput, 
                    "filename:1: error: Addressing mode of '%s' is not supported for '%s' instruction.\n", 
                    pAddressingModeStrings->pTestOperand,
                    pInstruction);
            sprintf(expectedListOutput, "    :              1 %s\n", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
        }
    }
    
    void testImmediateAddressing(const char* pInstruction)
    {
        static const AddressingModeStrings addressingModeStrings = {"#$ff", "FF   ", NULL};
        testAddressingMode(pInstruction, &addressingModeStrings);
    }
    
    void testAbsoluteAddressing(const char* pInstruction)
    {
        static const AddressingModeStrings addressingModeStrings = {"$100", "00 01", NULL};
        testAddressingMode(pInstruction, &addressingModeStrings);
    }
    
    void testZeroPageAddressing(const char* pInstruction)
    {
        static const AddressingModeStrings addressingModeStrings = {"$ff", "FF   ", "FF 00"};
        testAddressingMode(pInstruction, &addressingModeStrings);
    }
    
    void testImpliedAddressing(const char* pInstruction)
    {
        static const AddressingModeStrings addressingModeStrings = {"", "     ", "     "};
        testAddressingMode(pInstruction, &addressingModeStrings);
    }
    
    void testZeroPageIndexedIndirectAddressing(const char* pInstruction)
    {
        static const AddressingModeStrings addressingModeStrings = {"($ff,x)", "FF   ", "FF 00"};
        testAddressingMode(pInstruction, &addressingModeStrings);
    }
    
    void testIndirectIndexedAddressing(const char* pInstruction)
    {
        static const AddressingModeStrings addressingModeStrings = {"($ff),y", "FF   ", NULL};
        testAddressingMode(pInstruction, &addressingModeStrings);
    }
    
    void testZeroPageIndexedXAddressing(const char* pInstruction)
    {
        static const AddressingModeStrings addressingModeStrings = {"$ff,x", "FF   ", "FF 00"};
        testAddressingMode(pInstruction, &addressingModeStrings);
    }
    
    void testZeroPageIndexedYAddressing(const char* pInstruction)
    {
        static const AddressingModeStrings addressingModeStrings = {"$ff,y", "FF   ", "FF 00"};
        testAddressingMode(pInstruction, &addressingModeStrings);
    }
    
    void testAbsoluteIndexedXAddressing(const char* pInstruction)
    {
        static const AddressingModeStrings addressingModeStrings = {"$100,x", "00 01", NULL};
        testAddressingMode(pInstruction, &addressingModeStrings);
    }
    
    void testAbsoluteIndexedYAddressing(const char* pInstruction)
    {
        static const AddressingModeStrings addressingModeStrings = {"$100,y", "00 01", NULL};
        testAddressingMode(pInstruction, &addressingModeStrings);
    }
    
    void testRelativeAddressing(const char* pInstruction)
    {
        char               testString[64];
        char               expectedListOutput[128];

        CHECK_FALSE(m_isZeroPageTreatedAsAbsolute);
        if (m_isInvalidMode || m_isInvalidInstruction)
            return;
        
        sprintf(testString, " %s *\n", pInstruction);
        runAssembler(testString);
        sprintf(expectedListOutput, "8000: %02X FE        1 %s\n", m_expectedOpcode, testString);
        validateSuccessfulOutput(expectedListOutput);
    }
    
    void testAbsoluteIndirectAddressing(const char* pInstruction)
    {
        static const AddressingModeStrings addressingModeStrings = {"($100)", "00 01", NULL};
        testAddressingMode(pInstruction, &addressingModeStrings);
    }
    
    void testAbsoluteIndexedIndirectAddressing(const char* pInstruction)
    {
        static const AddressingModeStrings addressingModeStrings = {"($100,x)", "00 01", NULL};
        testAddressingMode(pInstruction, &addressingModeStrings);
    }
    
    void testZeroPageIndirectAddressing(const char* pInstruction)
    {
        static const AddressingModeStrings addressingModeStrings = {"($ff)", "FF   ", "FF 00"};
        testAddressingMode(pInstruction, &addressingModeStrings);
    }
    
    void runAssembler(char* pTestString)
    {
        clearOutputLogs();
        m_pAssembler = Assembler_CreateFromString(pTestString, NULL);
        if (m_switchTo65c02)
            m_pAssembler->instructionSet = INSTRUCTION_SET_65C02;
        Assembler_Run(m_pAssembler);
        Assembler_Free(m_pAssembler);
        m_pAssembler = NULL;
    }
    
    void clearOutputLogs()
    {
        printfSpy_Unhook();
        printfSpy_Hook(512);
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
