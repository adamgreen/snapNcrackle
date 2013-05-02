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
/* Tests for the core functionality of the Assembler module such as creating and destroying assembler objects, injection
   of file I/O and memory failures, basic expression evaluation, etc. 
*/
#include "AssemblerBaseTest.h"


TEST_GROUP_BASE(AssemblerCore, AssemblerBase)
{
};


TEST(AssemblerCore, FailAllInitAllocations)
{
    static const int allocationsToFail = 26;
    m_initParams.pListFilename = g_listFilename;
    m_initParams.pPutDirectories = ".";
    for (int i = 1 ; i <= allocationsToFail ; i++)
    {
        MallocFailureInject_FailAllocation(i);
        __try_and_catch( m_pAssembler = Assembler_CreateFromString(dupe(""), &m_initParams) );
        POINTERS_EQUAL(NULL, m_pAssembler);
        validateOutOfMemoryExceptionThrown();
    }

    MallocFailureInject_FailAllocation(allocationsToFail + 1);
    m_pAssembler = Assembler_CreateFromString(dupe(""), &m_initParams);
    CHECK_TRUE(m_pAssembler != NULL);
}

TEST(AssemblerCore, EmptyString)
{
    m_pAssembler = Assembler_CreateFromString(dupe(""), NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(0, printfSpy_GetCallCount());
}

TEST(AssemblerCore, InitFromEmptyFile)
{
    createSourceFile("");
    m_pAssembler = Assembler_CreateFromFile(g_sourceFilename, NULL);
    CHECK(m_pAssembler != NULL);
}

TEST(AssemblerCore, InitFromNonExistantFile)
{
    __try_and_catch( m_pAssembler = Assembler_CreateFromFile("foo.noexist.bar", NULL) );
    validateFileOpenExceptionThrown();
}

TEST(AssemblerCore, FailAllAllocationsDuringFileInit)
{
    static const int allocationsToFail = 27;
    createSourceFile(" ORG $800\r" LINE_ENDING);
    m_initParams.pListFilename = g_listFilename;
    m_initParams.pPutDirectories = ".";

    for (int i = 1 ; i <= allocationsToFail ; i++)
    {
        MallocFailureInject_FailAllocation(i);
        __try_and_catch( m_pAssembler = Assembler_CreateFromFile(g_sourceFilename, &m_initParams) );
        POINTERS_EQUAL(NULL, m_pAssembler);
        validateOutOfMemoryExceptionThrown();
    }

    MallocFailureInject_FailAllocation(allocationsToFail + 1);
    m_pAssembler = Assembler_CreateFromFile(g_sourceFilename, &m_initParams);
    CHECK_TRUE(m_pAssembler != NULL);
}

TEST(AssemblerCore, InitAndRunFromShortFile)
{
    createSourceFile("* Symbols" LINE_ENDING
                     "SYM1 = $1" LINE_ENDING
                     "SYM2 EQU $2" LINE_ENDING);
    m_pAssembler = Assembler_CreateFromFile(g_sourceFilename, NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(3, printfSpy_GetCallCount());
}

TEST(AssemblerCore, InitAndCreateActualListFileNotSentToStdOut)
{
    static const char expectedListOutput[] = "    :    =0001     1 SYM1 EQU $1" LINE_ENDING;
    createSourceFile("SYM1 EQU $1" LINE_ENDING);
    m_initParams.pListFilename = g_listFilename;

    printfSpy_Unhook();
    m_pAssembler = Assembler_CreateFromFile(g_sourceFilename, &m_initParams);
    Assembler_Run(m_pAssembler);
    Assembler_Free(m_pAssembler);
    m_pAssembler = NULL;

    validateListFileContains(expectedListOutput, sizeof(expectedListOutput)-1);
}

TEST(AssemblerCore, FailAttemptToOpenListFile)
{
    m_initParams.pListFilename = g_listFilename;
    fopenFail(NULL);
        __try_and_catch( m_pAssembler = Assembler_CreateFromString(dupe("* Comment line."), &m_initParams) );
    fopenRestore();
    LONGS_EQUAL(NULL, m_pAssembler);
    validateFileOpenExceptionThrown();
}

TEST(AssemblerCore, FailAllocationOnLineInfoAllocation)
{
    m_pAssembler = Assembler_CreateFromString(dupe("* Comment Line."), NULL);
    MallocFailureInject_FailAllocation(1);
        __try_and_catch( Assembler_Run(m_pAssembler) );
    LONGS_EQUAL(0, printfSpy_GetCallCount());
    validateOutOfMemoryExceptionThrown();
}

TEST(AssemblerCore, RunOnLongLine)
{
    char longLine[257];
    
    memset(longLine, ' ', sizeof(longLine));
    longLine[ARRAYSIZE(longLine)-1] = '\0';
    m_pAssembler = Assembler_CreateFromString(longLine, NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(1, printfSpy_GetCallCount());
    LONGS_EQUAL(0, Assembler_GetErrorCount(m_pAssembler) );
}

TEST(AssemblerCore, CommentLine)
{
    m_pAssembler = Assembler_CreateFromString(dupe("*  boot" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("    :              1 *  boot" LINE_ENDING);
}

TEST(AssemblerCore, InvalidOperatorFromStringSource)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" foo bar" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: 'foo' is not a recognized mnemonic or macro." LINE_ENDING, 
                                   "    :              1  foo bar" LINE_ENDING);
}

TEST(AssemblerCore, InvalidOperatorFromFileSource)
{
    createSourceFile(" foo bar" LINE_ENDING);
    m_pAssembler = Assembler_CreateFromFile(g_sourceFilename, NULL);
    runAssemblerAndValidateFailure("AssemblerTest.S:1: error: 'foo' is not a recognized mnemonic or macro." LINE_ENDING, 
                                   "    :              1  foo bar" LINE_ENDING);
}

TEST(AssemblerCore, Immediate16BitValueTruncatedToLower8BitByDefault)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lda #$100" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: A9 00        1  lda #$100" LINE_ENDING);
}

TEST(AssemblerCore, Immediate16BitValueTruncatedToLower8BitByLessThanPrefix)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lda #<$100" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: A9 00        1  lda #<$100" LINE_ENDING);
}

TEST(AssemblerCore, Immediate16BitValueWithGreaterThanPrefixToObtainHighByte)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lda #>$100" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: A9 01        1  lda #>$100" LINE_ENDING);
}

TEST(AssemblerCore, Immediate16BitValueWithCaretPrefixToObtainHighByte)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lda #^$100" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: A9 01        1  lda #^$100" LINE_ENDING);
}

TEST(AssemblerCore, FailWithInvalidExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta +ff" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '+ff' expression." LINE_ENDING,
                                   "    :              1  sta +ff" LINE_ENDING);
}

TEST(AssemblerCore, FailBinaryBufferAllocationOnEmitSingleByteInstruction)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" clc" LINE_ENDING), NULL);
    BinaryBuffer_FailAllocation(m_pAssembler->pCurrentBuffer, 1);
    runAssemblerAndValidateFailure("filename:1: error: Exceeded the 65536 allowed bytes in the object file." LINE_ENDING,
                                   "    :              1  clc" LINE_ENDING);
}

TEST(AssemblerCore, FailBinaryBufferAllocationOnEmitTwoByteInstruction)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lda #1" LINE_ENDING), NULL);
    BinaryBuffer_FailAllocation(m_pAssembler->pCurrentBuffer, 1);
    runAssemblerAndValidateFailure("filename:1: error: Exceeded the 65536 allowed bytes in the object file." LINE_ENDING,
                                   "    :              1  lda #1" LINE_ENDING);
}

TEST(AssemblerCore, FailBinaryBufferAllocationOnEmitThreeByteInstruction)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lda $800" LINE_ENDING), NULL);
    BinaryBuffer_FailAllocation(m_pAssembler->pCurrentBuffer, 1);
    runAssemblerAndValidateFailure("filename:1: error: Exceeded the 65536 allowed bytes in the object file." LINE_ENDING,
                                   "    :              1  lda $800" LINE_ENDING);
}
