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
    ListFile* m_pListFile;
    LineInfo  m_lineInfo;
    
    void setup()
    {
        clearExceptionCode();
        printfSpy_Hook(128);
        m_pListFile = NULL;
        LineInfo_Init(&m_lineInfo);
    }

    void teardown()
    {
        MallocFailureInject_Restore();
        ListFile_Free(m_pListFile);
        LineInfo_Free(&m_lineInfo);
        LONGS_EQUAL(noException, getExceptionCode());
        printfSpy_Unhook();
    }
};


TEST(ListFile, FailFirstAllocDuringCreate)
{
    MallocFailureInject_FailAllocation(1);
    m_pListFile = ListFile_Create(stdout);
    POINTERS_EQUAL(NULL, m_pListFile);
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}

TEST(ListFile, OutputLineWithOnlyLineNumberAndText)
{
    m_pListFile = ListFile_Create(stdout);

    LineInfo_SaveLineText(&m_lineInfo, "* Full line comment.");
    m_lineInfo.lineNumber = 1;
    ListFile_OutputLine(m_pListFile, &m_lineInfo);

    POINTERS_EQUAL(stdout, printfSpy_GetLastFile());
    STRCMP_EQUAL("    :              1 * Full line comment.\n", printfSpy_GetLastOutput());
}

TEST(ListFile, OutputLineWithSymbol)
{
    m_pListFile = ListFile_Create(stdout);

    LineInfo_SaveLineText(&m_lineInfo, "LABEL EQU $FFFF");
    m_lineInfo.lineNumber = 2;
    m_lineInfo.symbolValue = 0xFFFF;
    m_lineInfo.validSymbol = 1;
    ListFile_OutputLine(m_pListFile, &m_lineInfo);

    POINTERS_EQUAL(stdout, printfSpy_GetLastFile());
    STRCMP_EQUAL("    :    =FFFF     2 LABEL EQU $FFFF\n", printfSpy_GetLastOutput());
}

TEST(ListFile, OutputLineWithAddressAndOneMachineCodeByte)
{
    m_pListFile = ListFile_Create(stderr);

    LineInfo_SaveLineText(&m_lineInfo, " DEX");
    m_lineInfo.lineNumber = 3;
    m_lineInfo.address = 0x0800;
    m_lineInfo.machineCodeSize = 1;
    m_lineInfo.machineCode[0] = 0xCA;
    ListFile_OutputLine(m_pListFile, &m_lineInfo);

    POINTERS_EQUAL(stderr, printfSpy_GetLastFile());
    STRCMP_EQUAL("0800: CA           3  DEX\n", printfSpy_GetLastOutput());
}

TEST(ListFile, OutputLineWithAddressAndTwoMachineCodeBytes)
{
    m_pListFile = ListFile_Create(stderr);

    LineInfo_SaveLineText(&m_lineInfo, " LDA $2C");
    m_lineInfo.lineNumber = 4;
    m_lineInfo.address = 0x0801;
    m_lineInfo.machineCodeSize = 2;
    m_lineInfo.machineCode[0] = 0xA5;
    m_lineInfo.machineCode[1] = 0x2C;
    ListFile_OutputLine(m_pListFile, &m_lineInfo);

    POINTERS_EQUAL(stderr, printfSpy_GetLastFile());
    STRCMP_EQUAL("0801: A5 2C        4  LDA $2C\n", printfSpy_GetLastOutput());
}

TEST(ListFile, OutputLineWithAddressAndThreeMachineCodeBytes)
{
    m_pListFile = ListFile_Create(stdout);

    LineInfo_SaveLineText(&m_lineInfo, " LDA $C008");
    m_lineInfo.lineNumber = 5;
    m_lineInfo.address = 0x0803;
    m_lineInfo.machineCodeSize = 3;
    m_lineInfo.machineCode[0] = 0xAD;
    m_lineInfo.machineCode[1] = 0xC0;
    m_lineInfo.machineCode[2] = 0x08;
    ListFile_OutputLine(m_pListFile, &m_lineInfo);

    POINTERS_EQUAL(stdout, printfSpy_GetLastFile());
    STRCMP_EQUAL("0803: AD C0 08     5  LDA $C008\n", printfSpy_GetLastOutput());
}
