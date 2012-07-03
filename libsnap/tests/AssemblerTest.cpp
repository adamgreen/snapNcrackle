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
    #include "CommandLine.h"
    #include "Assembler.h"
    #include "MallocFailureInject.h"
    #include "printfSpy.h"
    #include "util.h"
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


const char* g_sourceFilename = "AssemblerTest.S";

TEST_GROUP(Assembler)
{
    Assembler*  m_pAssembler;
    const char* m_argv[10];
    CommandLine m_commandLine;
    int         m_argc;
    char        m_buffer[128];
    
    void setup()
    {
        clearExceptionCode();
        printfSpy_Hook(128);
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
        Assembler_Run(m_pAssembler);
        LONGS_EQUAL(1, printfSpy_GetCallCount());
        STRCMP_EQUAL(pExpectedOutput, printfSpy_GetLastOutput());
        LONGS_EQUAL(0, Assembler_GetErrorCount(m_pAssembler) );
    }
    
    void runAssemblerAndValidateFailure(const char* pExpectedFailureMessage, const char* pExpectedListOutput)
    {
        Assembler_Run(m_pAssembler);
        LONGS_EQUAL(2, printfSpy_GetCallCount());
        STRCMP_EQUAL(pExpectedFailureMessage, printfSpy_GetPreviousOutput());
        STRCMP_EQUAL(pExpectedListOutput, printfSpy_GetLastOutput());
        LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler) );
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

// UNDONE: Turn this test back on once ORG and INX are supported.
IGNORE_TEST(Assembler, InitAndRunFromShortFile)
{
    createSourceFile(" ORG $800\r\n"
                     " INX\r\n"
                     "* Comment\r\n");
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

TEST(Assembler, CommentLine)
{
    m_pAssembler = Assembler_CreateFromString(dupe("*  boot\n"));
    runAssemblerAndValidateOutputIs("    :              1 *  boot\n");
}

TEST(Assembler, InvalidOperatorFromStringSource)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" foo bar\n"));
    runAssemblerAndValidateFailure("filename:1: error: 'foo' is not a recongized directive, mnemonic, or macro.\n", 
                                   "    :              1  foo bar\n");
}

TEST(Assembler, InvalidOperatorFromFileSource)
{
    createSourceFile(" foo bar\n");
    m_pAssembler = Assembler_CreateFromFile(g_sourceFilename);
    runAssemblerAndValidateFailure("AssemblerTest.S:1: error: 'foo' is not a recongized directive, mnemonic, or macro.\n", 
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

TEST(Assembler, NotHexExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe("org EQU 800\n"));
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '800' expression.\n", 
                                   "    :              1 org EQU 800\n");
}

TEST(Assembler, PERIODinExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe("CHECKEND = *-CHECKER\n"));
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '*-CHECKER' expression.\n", 
                                   "    :              1 CHECKEND = *-CHECKER\n");
}

TEST(Assembler, IgnoreLSTDirective)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lst off\n"));
    runAssemblerAndValidateOutputIs("    :              1  lst off\n");
}
