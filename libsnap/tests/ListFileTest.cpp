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
    #include "ListFile.h"
    #include "MallocFailureInject.h"
    #include "printfSpy.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(ListFile)
{
    ListFile*     m_pListFile;
    LineInfo      m_lineInfo;
    Symbol        m_symbol;
    unsigned char m_machineCode[32];
    
    void setup()
    {
        clearExceptionCode();
        printfSpy_Hook(128);
        m_pListFile = NULL;
        memset(&m_symbol, 0, sizeof(m_symbol));
        memset(&m_lineInfo, 0, sizeof(m_lineInfo));
        m_lineInfo.pMachineCode = m_machineCode;
        m_pListFile = ListFile_Create(stdout);
    }

    void teardown()
    {
        MallocFailureInject_Restore();
        ListFile_Free(m_pListFile);
        LONGS_EQUAL(noException, getExceptionCode());
        printfSpy_Unhook();
    }
};


TEST(ListFile, FailFirstAllocDuringCreate)
{
    ListFile_Free(m_pListFile);
    m_pListFile = NULL;
    
    MallocFailureInject_FailAllocation(1);
        __try_and_catch( m_pListFile = ListFile_Create(stdout) );
    
    POINTERS_EQUAL(NULL, m_pListFile);
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}

TEST(ListFile, OutputLineWithOnlyLineNumberAndText)
{
    m_lineInfo.pLineText = "* Full line comment.";
    m_lineInfo.lineNumber = 1;
    ListFile_OutputLine(m_pListFile, &m_lineInfo);

    POINTERS_EQUAL(stdout, printfSpy_GetLastFile());
    STRCMP_EQUAL("    :              1 * Full line comment.\n", printfSpy_GetLastOutput());
}

TEST(ListFile, OutputLineWithOnlyLineNumberAndTextToStderr)
{
    ListFile_Free(m_pListFile);
    m_pListFile = ListFile_Create(stderr);
    
    m_lineInfo.pLineText = "* Full line comment.";
    m_lineInfo.lineNumber = 1;
    ListFile_OutputLine(m_pListFile, &m_lineInfo);

    POINTERS_EQUAL(stderr, printfSpy_GetLastFile());
    STRCMP_EQUAL("    :              1 * Full line comment.\n", printfSpy_GetLastOutput());
}

TEST(ListFile, OutputLineWithSymbol)
{
    m_lineInfo.pLineText = "LABEL EQU $FFFF";
    m_lineInfo.lineNumber = 2;
    m_lineInfo.flags |= LINEINFO_FLAG_WAS_EQU;
    m_symbol.expression.value = 0xFFFF;
    m_lineInfo.pSymbol = &m_symbol;
    ListFile_OutputLine(m_pListFile, &m_lineInfo);

    STRCMP_EQUAL("    :    =FFFF     2 LABEL EQU $FFFF\n", printfSpy_GetLastOutput());
}

TEST(ListFile, OutputLineWithAddressAndOneMachineCodeByte)
{
    m_lineInfo.pLineText = " DEX";
    m_lineInfo.lineNumber = 3;
    m_lineInfo.address = 0x0800;
    m_lineInfo.machineCodeSize = 1;
    m_lineInfo.pMachineCode[0] = 0xCA;
    ListFile_OutputLine(m_pListFile, &m_lineInfo);

    STRCMP_EQUAL("0800: CA           3  DEX\n", printfSpy_GetLastOutput());
}

TEST(ListFile, OutputLineWithAddressAndTwoMachineCodeBytes)
{
    m_lineInfo.pLineText = " LDA $2C";
    m_lineInfo.lineNumber = 4;
    m_lineInfo.address = 0x0801;
    m_lineInfo.machineCodeSize = 2;
    m_lineInfo.pMachineCode[0] = 0xA5;
    m_lineInfo.pMachineCode[1] = 0x2C;
    ListFile_OutputLine(m_pListFile, &m_lineInfo);

    STRCMP_EQUAL("0801: A5 2C        4  LDA $2C\n", printfSpy_GetLastOutput());
}

TEST(ListFile, OutputLineWithAddressAndThreeMachineCodeBytes)
{
    m_lineInfo.pLineText = " LDA $C008";
    m_lineInfo.lineNumber = 5;
    m_lineInfo.address = 0x0803;
    m_lineInfo.machineCodeSize = 3;
    m_lineInfo.pMachineCode[0] = 0xAD;
    m_lineInfo.pMachineCode[1] = 0xC0;
    m_lineInfo.pMachineCode[2] = 0x08;
    ListFile_OutputLine(m_pListFile, &m_lineInfo);

    STRCMP_EQUAL("0803: AD C0 08     5  LDA $C008\n", printfSpy_GetLastOutput());
}

TEST(ListFile, OutputLineWithAddressAndFourMachineCodeBytes)
{
    m_lineInfo.pLineText = " DS 4";
    m_lineInfo.lineNumber = 1;
    m_lineInfo.address = 0x0800;
    m_lineInfo.machineCodeSize = 4;
    m_lineInfo.pMachineCode[0] = 0x00;
    m_lineInfo.pMachineCode[1] = 0x00;
    m_lineInfo.pMachineCode[2] = 0x00;
    m_lineInfo.pMachineCode[3] = 0x00;
    ListFile_OutputLine(m_pListFile, &m_lineInfo);

    STRCMP_EQUAL("0800: 00 00 00     1  DS 4\n", printfSpy_GetPreviousOutput());
    STRCMP_EQUAL("0803: 00      \n", printfSpy_GetLastOutput());
}
