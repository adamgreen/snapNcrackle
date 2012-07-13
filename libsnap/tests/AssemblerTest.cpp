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
    Assembler*  m_pAssembler;
    const char* m_argv[10];
    CommandLine m_commandLine;
    int         m_argc;
    char        m_buffer[128];
    
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
    
    runAssemblerAndValidateFailure("filename:2: error: Couldn't properly infer size of 'globalLabel' forward reference.\n",
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

TEST(Assembler, LDAImmediate)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lda #$60\n"));
    runAssemblerAndValidateOutputIs("0000: A9 60        1  lda #$60\n");
}

TEST(Assembler, LDAInvalidImmediateValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lda #$100\n"));
    runAssemblerAndValidateFailure("filename:1: error: Immediate expression '$100' doesn't fit in 8-bits.\n",
                                   "    :              1  lda #$100\n");
}

TEST(Assembler, LDAAbsoluteNotYetImplemented)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lda $900\n"));
    runAssemblerAndValidateFailure("filename:1: error: '$900' specifies invalid addressing mode for this instruction.\n",
                                   "    :              1  lda $900\n");
}

TEST(Assembler, STAAbsolute)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta $4fb\n"));
    runAssemblerAndValidateOutputIs("0000: 8D FB 04     1  sta $4fb\n");
}

TEST(Assembler, STAZeroPageAbsolute)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta $fb\n"));
    runAssemblerAndValidateOutputIs("0000: 85 FB        1  sta $fb\n");
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

TEST(Assembler, STAInvalidImmediateValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta #$ff\n"));
    runAssemblerAndValidateFailure("filename:1: error: '#$ff' specifies invalid addressing mode for this instruction.\n",
                                   "    :              1  sta #$ff\n");
}

TEST(Assembler, STAInvalidInvalidExpress)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta +ff\n"));
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '+ff' expression.\n",
                                   "    :              1  sta +ff\n");
}

TEST(Assembler, JSRAbsolute)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" jsr $180\n"));
    runAssemblerAndValidateOutputIs("0000: 20 80 01     1  jsr $180\n");
}

TEST(Assembler, JSRAbsoluteToPageZeroWhichIsStillAbsolute)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" jsr $005c\n"));
    runAssemblerAndValidateOutputIs("0000: 20 5C 00     1  jsr $005c\n");
}

TEST(Assembler, JSRToAbsoluteEQU)
{
    m_pAssembler = Assembler_CreateFromString(dupe("text = $fb2f\n"
                                                   " jsr text\n"));
    runAssemblerAndValidateLastLineIs("0000: 20 2F FB     2  jsr text\n", 2);
}

TEST(Assembler, JSRWithInvalidAddressingMode)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" jsr #$5c\n"));
    runAssemblerAndValidateFailure("filename:1: error: '#$5c' specifies invalid addressing mode for this instruction.\n",
                                   "    :              1  jsr #$5c\n");
}

TEST(Assembler, JSRWithInvalidExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" jsr +ff\n"));
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '+ff' expression.\n",
                                   "    :              1  jsr +ff\n");
}

TEST(Assembler, LDXAbsolute)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ldx $100\n"));
    runAssemblerAndValidateOutputIs("0000: AE 00 01     1  ldx $100\n");
}

TEST(Assembler, LDXZeroPageAbsolute)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ldx $2b\n"));
    runAssemblerAndValidateOutputIs("0000: A6 2B        1  ldx $2b\n");
}

TEST(Assembler, LDXAbsoluteSymbol)
{
    m_pAssembler = Assembler_CreateFromString(dupe("label equ $100\n"
                                                   " ldx label\n"));
    runAssemblerAndValidateLastLineIs("0000: AE 00 01     2  ldx label\n", 2);
}

TEST(Assembler, LDXZeroPageAbsoluteSymbol)
{
    m_pAssembler = Assembler_CreateFromString(dupe("SLOT = $2b\n"
                                                   " ldx SLOT\n"));
    runAssemblerAndValidateLastLineIs("0000: A6 2B        2  ldx SLOT\n", 2);
}

TEST(Assembler, LDXWithInvalidExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ldx +ff\n"));
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '+ff' expression.\n",
                                   "    :              1  ldx +ff\n");
}

TEST(Assembler, LDXWithInvalidAddressingMode)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ldx #$5c\n"));
    runAssemblerAndValidateFailure("filename:1: error: '#$5c' specifies invalid addressing mode for this instruction.\n",
                                   "    :              1  ldx #$5c\n");
}

TEST(Assembler, TXA)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" txa\n"));
    runAssemblerAndValidateOutputIs("0000: 8A           1  txa\n");
}

TEST(Assembler, LSRAccumulator)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lsr\n"));
    runAssemblerAndValidateOutputIs("0000: 4A           1  lsr\n");
}

TEST(Assembler, LSRInvalidAddressMode)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lsr #$5c\n"));
    runAssemblerAndValidateFailure("filename:1: error: '#$5c' specifies invalid addressing mode for this instruction.\n",
                                   "    :              1  lsr #$5c\n");
}

TEST(Assembler, LSRNotYetImplementedAddressMode)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lsr $5c\n"));
    runAssemblerAndValidateFailure("filename:1: error: '$5c' specifies invalid addressing mode for this instruction.\n",
                                   "    :              1  lsr $5c\n");
}

TEST(Assembler, ORAImmediate)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ora #$c0\n"));
    runAssemblerAndValidateOutputIs("0000: 09 C0        1  ora #$c0\n");
}

TEST(Assembler, ORAWithInvalidExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ora +ff\n"));
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '+ff' expression.\n",
                                   "    :              1  ora +ff\n");
}

TEST(Assembler, ORANotYetImplementedAddressMode)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ora $c0\n"));
    runAssemblerAndValidateFailure("filename:1: error: '$c0' specifies invalid addressing mode for this instruction.\n",
                                   "    :              1  ora $c0\n");
}
