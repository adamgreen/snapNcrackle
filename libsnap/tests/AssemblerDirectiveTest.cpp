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
/* Tests for the assembler directives.
   NOTE: EQU directive is tested as part of the AssemblerLabelTest set of unit tests.
*/
#include "AssemblerBaseTest.h"


static const char* g_putFilename = "AssemblerTestPut.S";
static const char* g_putFilename2 = "AssemblerTestPut2.S";
static const char* g_usrFilename = "AssemblerTest.usr";


TEST_GROUP_BASE(AssemblerDirectives, AssemblerBase)
{
    void teardown()
    {
        fopenRestore();
        remove(g_putFilename);
        remove(g_putFilename2);
        remove(g_usrFilename);
        remove("AssemblerTest");
        AssemblerBase::teardown();
    }
};


TEST(AssemblerDirectives, IgnoreLSTDirective)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lst off\n"), NULL);
    runAssemblerAndValidateOutputIs("    :              1  lst off\n");
}

TEST(AssemblerDirectives, HEXDirectiveWithSingleValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 01\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: 01           1  hex 01\n");
}

TEST(AssemblerDirectives, HEXDirectiveWithMixedCase)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex cD,Cd\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: CD CD        1  hex cD,Cd\n");
}

TEST(AssemblerDirectives, HEXDirectiveWithSingleValueAndFollowedByComment)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 01 ; Comment\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: 01           1  hex 01 ; Comment\n");
}

TEST(AssemblerDirectives, HEXDirectiveWithThreeValuesAndCommas)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 0e,0c,0a\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: 0E 0C 0A     1  hex 0e,0c,0a\n");
}

TEST(AssemblerDirectives, HEXDirectiveWithThreeValues)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 0e0c0a\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: 0E 0C 0A     1  hex 0e0c0a\n");
}

TEST(AssemblerDirectives, HEXDirectiveWithFourValues)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 01,02,03,04\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 01 02 03     1  hex 01,02,03,04\n",
                                                   "8003: 04      \n");
}

TEST(AssemblerDirectives, HEXDirectiveWithSixValues)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 01,02,03,04,05,06\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 01 02 03     1  hex 01,02,03,04,05,06\n",
                                                   "8003: 04 05 06\n");
}

TEST(AssemblerDirectives, HEXDirectiveWithMaximumOf32Value)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20\n"),
                                              NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("801B: 1C 1D 1E\n",
                                                   "801E: 1F 20   \n", 11);
}

TEST(AssemblerDirectives, HEXDirectiveWith33Values_1MoreThanSupported)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f2021\n"),
                                              NULL);
    runAssemblerAndValidateFailure("filename:1: error: '0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f2021' contains more than 32 values.\n", 
                                   "801E: 1F 20   \n", 12);
}

TEST(AssemblerDirectives, HEXDirectiveOnTwoLines)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 01\n"
                                                   " hex 02\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 01           1  hex 01\n",
                                                   "8001: 02           2  hex 02\n");
}

TEST(AssemblerDirectives, HEXDirectiveWithUpperCaseHex)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex FA\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: FA           1  hex FA\n");
}

TEST(AssemblerDirectives, HEXDirectiveWithLowerCaseHex)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex fa\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: FA           1  hex fa\n");
}

TEST(AssemblerDirectives, HEXDirectiveWithOddDigitCount)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex fa0\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: 'fa0' doesn't contain an even number of hex digits.\n",
                                   "    :              1  hex fa0\n");
}

TEST(AssemblerDirectives, HEXDirectiveWithInvalidDigit)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex fg\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: 'fg' contains an invalid hex digit.\n",
                                   "    :              1  hex fg\n");
}

TEST(AssemblerDirectives, HEXDirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: hex directive requires operand.\n",
                                   "    :              1  hex\n", 2);
}

TEST(AssemblerDirectives, FailBinaryBufferAllocationInHEXDirective)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex ff\n"), NULL);
    BinaryBuffer_FailAllocation(m_pAssembler->pCurrentBuffer, 1);
    runAssemblerAndValidateFailure("filename:1: error: Exceeded the 65536 allowed bytes in the object file.\n",
                                   "    :              1  hex ff\n");
}

TEST(AssemblerDirectives, ORGDirectiveWithLiteralValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $900\n"
                                                   " hex 01\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              1  org $900\n", 
                                                   "0900: 01           2  hex 01\n");
}

TEST(AssemblerDirectives, ORGDirectiveWithInvalidExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org +900\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '+900' expression.\n",
                                   "    :              1  org +900\n");
}

TEST(AssemblerDirectives, ORGDirectiveWithSymbolValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe("org = $800\n"
                                                   " org org\n"
                                                   " hex 01\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              2  org org\n",
                                                   "0800: 01           3  hex 01\n", 3);
}

TEST(AssemblerDirectives, ORGDirectiveWithInvalidImmediate)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org #$00\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: '#$00' doesn't specify an absolute address.\n",
                                   "    :              1  org #$00\n");
}

TEST(AssemblerDirectives, ORGDirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: org directive requires operand.\n",
                                   "    :              1  org\n", 2);
}

TEST(AssemblerDirectives, DUMandDEND_Directive)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " dum $00\n"
                                                   " hex ff\n"
                                                   " dend\n"
                                                   " hex fe\n"), NULL);
    Assembler_Run(m_pAssembler);
    
    LineInfo* pThirdLine = m_pAssembler->linesHead.pNext->pNext->pNext;
    LineInfo* pFifthLine = pThirdLine->pNext->pNext;
    
    validateLineInfo(pThirdLine, 0x0000, 1, "\xff");
    validateLineInfo(pFifthLine, 0x0800, 1, "\xfe");
}

TEST(AssemblerDirectives, TwoDUMandDEND_Directive)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " dum $00\n"
                                                   " hex ff\n"
                                                   " dum $100\n"
                                                   " hex fe\n"
                                                   " dend\n"
                                                   " hex fd\n"), NULL);
    Assembler_Run(m_pAssembler);
    
    LineInfo* pThirdLine = m_pAssembler->linesHead.pNext->pNext->pNext;
    LineInfo* pFifthLine = pThirdLine->pNext->pNext;
    LineInfo* pSeventhLine = pFifthLine->pNext->pNext;
    
    validateLineInfo(pThirdLine, 0x0000, 1, "\xff");
    validateLineInfo(pFifthLine, 0x0100, 1, "\xfe");
    validateLineInfo(pSeventhLine, 0x0800, 1, "\xfd");
}

TEST(AssemblerDirectives, DUM_ORG_andDEND_Directive)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " dum $00\n"
                                                   " hex ff\n"
                                                   " org $100\n"
                                                   " hex fe\n"
                                                   " dend\n"
                                                   " hex fd\n"), NULL);
    Assembler_Run(m_pAssembler);
    
    LineInfo* pThirdLine = m_pAssembler->linesHead.pNext->pNext->pNext;
    LineInfo* pFifthLine = pThirdLine->pNext->pNext;
    LineInfo* pSeventhLine = pFifthLine->pNext->pNext;
    
    validateLineInfo(pThirdLine, 0x0000, 1, "\xff");
    validateLineInfo(pFifthLine, 0x0100, 1, "\xfe");
    validateLineInfo(pSeventhLine, 0x0800, 1, "\xfd");
}

TEST(AssemblerDirectives, DUMDirectiveWithInvalidImmediateExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dum #$00\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: '#$00' doesn't specify an absolute address.\n",
                                   "    :              1  dum #$00\n");
}

TEST(AssemblerDirectives, DEND_DirectiveWithoutDUM)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dend\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: dend isn't allowed without a preceding DUM directive.\n",
                                   "    :              1  dend\n");
}

TEST(AssemblerDirectives, DUMDirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dum\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: dum directive requires operand.\n",
                                   "    :              1  dum\n", 2);
}

TEST(AssemblerDirectives, DENDDirectiveWithOperandWhenNotExpected)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dend $100\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: dend directive doesn't require operand.\n",
                                   "    :              1  dend $100\n", 2);
}

TEST(AssemblerDirectives, DS_DirectiveWithSmallRepeatValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds 1\n"
                                                   " hex ff\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 00           1  ds 1\n", 
                                                   "8001: FF           2  hex ff\n");
}

TEST(AssemblerDirectives, DS_DirectiveWithRepeatValueGreaterThan32)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds 42\n"
                                                   " hex ff\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8027: 00 00 00\n", 
                                                   "802A: FF           2  hex ff\n", 15);
}

TEST(AssemblerDirectives, DS_DirectiveWithBackSlashWhenAlreadyOnPageBoundary)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds \\\n"), NULL);
    runAssemblerAndValidateOutputIs("    :              1  ds \\\n");
}

TEST(AssemblerDirectives, DS_DirectiveWithBackSlashWhenOneByteFromPageBoundary)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds 255\n"
                                                   " ds \\,$ff\n"), NULL);
    runAssemblerAndValidateLastLineIs("80FF: FF           2  ds \\,$ff\n", 86);
}

TEST(AssemblerDirectives, DS_DirectiveWithSecondExpressionToSpecifyFillValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds 2,$ff\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: FF FF        1  ds 2,$ff\n");
}

TEST(AssemblerDirectives, DS_DirectiveWithInvalidExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds ($800\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '($800' expression.\n",
                                   "    :              1  ds ($800\n");
}

TEST(AssemblerDirectives, DS_DirectiveWithForwardReference)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds \\,Fill\n"
                                                   "Fill equ $ff\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              1  ds \\,Fill\n",
                                                   "    :    =00FF     2 Fill equ $ff\n");
}

TEST(AssemblerDirectives, DS_DirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: ds directive requires operand.\n",
                                   "    :              1  ds\n", 2);
}

TEST(AssemblerDirectives, FailBinaryBufferAllocationInDSDirective)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds 1\n"), NULL);
    BinaryBuffer_FailAllocation(m_pAssembler->pCurrentBuffer, 1);
    runAssemblerAndValidateFailure("filename:1: error: Exceeded the 65536 allowed bytes in the object file.\n",
                                   "    :              1  ds 1\n");
}

TEST(AssemblerDirectives, ASC_DirectiveInDoubleQuotes)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" asc \"Tst\"\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: D4 F3 F4     1  asc \"Tst\"\n");
}

TEST(AssemblerDirectives, ASC_DirectiveInDoubleQuotesAndFollowedByComment)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" asc \"Tst\" ;Comment\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: D4 F3 F4     1  asc \"Tst\" ;Comment\n");
}

TEST(AssemblerDirectives, ASC_DirectiveInSingleQuotes)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" asc 'Tst'\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: 54 73 74     1  asc 'Tst'\n");
}

TEST(AssemblerDirectives, ASC_DirectiveWithNoSpacesBetweenQuotes)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" asc 'a b'\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: 61 20 62     1  asc 'a b'\n");
}

TEST(AssemblerDirectives, ASC_DirectiveWithNoEndingDelimiter)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" asc 'Tst\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: 'Tst didn't end with the expected ' delimiter.\n",
                                   "8000: 54 73 74     1  asc 'Tst\n");
}

TEST(AssemblerDirectives, ASC_DirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" asc\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: asc directive requires operand.\n",
                                   "    :              1  asc\n", 2);
}

TEST(AssemblerDirectives, FailBinaryBufferAllocationInASCDirective)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" asc 'Tst'\n"), NULL);
    BinaryBuffer_FailAllocation(m_pAssembler->pCurrentBuffer, 1);
    runAssemblerAndValidateFailure("filename:1: error: Exceeded the 65536 allowed bytes in the object file.\n",
                                   "    :              1  asc 'Tst'\n");
}

TEST(AssemblerDirectives, SAV_DirectiveOnEmptyObjectFile)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sav AssemblerTest.sav\n"), NULL);
    runAssemblerAndValidateOutputIs("    :              1  sav AssemblerTest.sav\n");
    validateObjectFileContains(0x8000, "", 0);
}

TEST(AssemblerDirectives, SAV_DirectiveOnSmallObjectFile)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " hex 00,ff\n"
                                                   " sav AssemblerTest.sav\n"), NULL);
    runAssemblerAndValidateLastLineIs("    :              3  sav AssemblerTest.sav\n", 3);
    validateObjectFileContains(0x800, "\x00\xff", 2);
}

TEST(AssemblerDirectives, SAV_DirectiveOnSmallObjectFileOverridingOutputDirectoryWithoutSlash)
{
    m_initParams.pOutputDirectory = ".";
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " hex 00,ff\n"
                                                   " sav AssemblerTest.sav\n"), &m_initParams);
    runAssemblerAndValidateLastLineIs("    :              3  sav AssemblerTest.sav\n", 3);
    validateObjectFileContains(0x800, "\x00\xff", 2);
}

TEST(AssemblerDirectives, SAV_DirectiveOnSmallObjectFileOverridingOutputDirectoryWithSlash)
{
    m_initParams.pOutputDirectory = "./";
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " hex 00,ff\n"
                                                   " sav AssemblerTest.sav\n"), &m_initParams);
    runAssemblerAndValidateLastLineIs("    :              3  sav AssemblerTest.sav\n", 3);
    validateObjectFileContains(0x800, "\x00\xff", 2);
}

TEST(AssemblerDirectives, SAV_DirectiveOnSmallObjectFileOverridingToInvalidOutputDirectory)
{
    m_initParams.pOutputDirectory = "foobar/";
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " hex 00,ff\n"
                                                   " sav AssemblerTest.sav\n"), &m_initParams);
    __try_and_catch( Assembler_Run(m_pAssembler) );
    STRCMP_EQUAL("filename:3: error: Failed to save output.\n", printfSpy_GetLastOutput());
    validateExceptionThrown(fileException);
}

TEST(AssemblerDirectives, SAV_DirectiveShouldBeIgnoredOnErrors)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " hex 00,ff\n"
                                                   " hex ($00)\n"
                                                   " sav AssemblerTest.sav\n"), NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    m_pFile = fopen(g_objectFilename, "r");
    POINTERS_EQUAL(NULL, m_pFile);
}

TEST(AssemblerDirectives, SAV_DirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sav\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: sav directive requires operand.\n",
                                   "    :              1  sav\n", 2);
}

TEST(AssemblerDirectives, SAV_DirectiveFailWriteFileQueue)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sav AssemblerTest.sav\n"), NULL);
    MallocFailureInject_FailAllocation(2);
    runAssemblerAndValidateFailure("filename:1: error: Failed to queue up save to 'AssemblerTest.sav'.\n",
                                   "    :              1  sav AssemblerTest.sav\n");
}

TEST(AssemblerDirectives, DB_DirectiveWithSingleExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe("Value EQU $fe\n"
                                                   " db Value+1\n"), NULL);
    runAssemblerAndValidateLastLineIs("8000: FF           2  db Value+1\n", 2);
}

TEST(AssemblerDirectives, DB_DirectiveWithSingleExpressionAndComment)
{
    m_pAssembler = Assembler_CreateFromString(dupe("Value EQU $fe\n"
                                                   " db Value+1 ;Comment\n"), NULL);
    runAssemblerAndValidateLastLineIs("8000: FF           2  db Value+1 ;Comment\n", 2);
}

TEST(AssemblerDirectives, DB_DirectiveWithThreeExpressions)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" db 2,0,1\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: 02 00 01     1  db 2,0,1\n");
}

TEST(AssemblerDirectives, DB_DirectiveWithImmediateExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" db #$ff\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: FF           1  db #$ff\n");
}

TEST(AssemblerDirectives, DB_DirectiveWithForwardReference)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" db Label\n"
                                                   "Label db $12\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 01           1  db Label\n",
                                                   "8001: 12           2 Label db $12\n");
}

TEST(AssemblerDirectives, DB_DirectiveWithInvalidExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" db ($800\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '($800' expression.\n",
                                   "    :              1  db ($800\n");
}

TEST(AssemblerDirectives, DB_DirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" db\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: db directive requires operand.\n",
                                   "    :              1  db\n", 2);
}

TEST(AssemblerDirectives, DFB_DirectiveSameAsDB)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dfb 2,0,1\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: 02 00 01     1  dfb 2,0,1\n");
}

TEST(AssemblerDirectives, DFB_DirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dfb\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: dfb directive requires operand.\n",
                                   "    :              1  dfb\n", 2);
}

TEST(AssemblerDirectives, TR_DirectiveIsIgnored)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" tr on\n"), NULL);
    runAssemblerAndValidateOutputIs("    :              1  tr on\n");
}

TEST(AssemblerDirectives, DA_DirectiveWithOneExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" da $ff+1\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: 00 01        1  da $ff+1\n");
}

TEST(AssemblerDirectives, DA_DirectiveWithOneExpressionAndComment)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" da $ff+1 ;Comment\n"), NULL);
    runAssemblerAndValidateOutputIs("8000: 00 01        1  da $ff+1 ;Comment\n");
}

TEST(AssemblerDirectives, DA_DirectiveWithThreeExpressions)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" da $ff+1,$ff,$1233+1\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 00 01 FF     1  da $ff+1,$ff,$1233+1\n",
                                                   "8003: 00 34 12\n");
}

TEST(AssemblerDirectives, DA_DirectiveWithForwardReference)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" da Label\n"
                                                   "Label da $1234\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 02 80        1  da Label\n",
                                                   "8002: 34 12        2 Label da $1234\n");
}

TEST(AssemblerDirectives, DA_DirectiveWithInvalidExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" da ($800\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '($800' expression.\n",
                                   "    :              1  da ($800\n");
}

TEST(AssemblerDirectives, DA_DirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" da\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: da directive requires operand.\n",
                                   "    :              1  da\n", 2);
}

TEST(AssemblerDirectives, DW_DirectiveSameAsDA)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dw $ff+1,$ff,$1233+1\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 00 01 FF     1  dw $ff+1,$ff,$1233+1\n",
                                                   "8003: 00 34 12\n");
}

TEST(AssemblerDirectives, DW_DirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dw\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: dw directive requires operand.\n",
                                   "    :              1  dw\n", 2);
}

/* UNDONE: This should test that a 65C02 instruction is allowed after issue. */
TEST(AssemblerDirectives, XC_DirectiveSingleInvocation)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" xc\n"), NULL);
    runAssemblerAndValidateOutputIs("    :              1  xc\n");
}

/* UNDONE: This is testing a temporary hack to ignore 65802/65816 instructions as I don't support those at this time. */
TEST(AssemblerDirectives, XC_DirectiveTwiceShouldCauseRTSInsertion)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" xc\n"
                                                   " xc\n"
                                                   " lda #20\n"), NULL);
    runAssemblerAndValidateLastLineIs("8000: 60           3  lda #20\n", 3);
}

TEST(AssemblerDirectives, XC_DirectiveTwiceShouldCauseRTSInsertionForUnknownOpcodes)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" xc\n"
                                                   " xc\n"
                                                   " foobar\n"), NULL);
    runAssemblerAndValidateLastLineIs("8000: 60           3  foobar\n", 3);
}

TEST(AssemblerDirectives, XC_DirectiveWithOffOperandShouldResetTo6502)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" xc\n"
                                                   " xc\n"
                                                   " xc off\n"
                                                   " lda #20\n"), NULL);
    runAssemblerAndValidateLastLineIs("8000: A9 14        4  lda #20\n", 4);
}

TEST(AssemblerDirectives, XC_DirectiveThriceShouldCauseError)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" xc\n"
                                                   " xc\n"
                                                   " xc\n"), NULL);
    runAssemblerAndValidateFailure("filename:3: error: Can't have more than 2 XC directives.\n",
                                   "    :              3  xc\n", 4);
}

TEST(AssemblerDirectives, XC_DirectiveForwardReferenceFrom6502To65816)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lda #ForwardLabel\n"
                                                   " xc\n"
                                                   " xc\n"
                                                   "ForwardLabel lda #20\n"), NULL);
    runAssemblerAndValidateLastLineIs("8002: 60           4 ForwardLabel lda #20\n", 4);
}

TEST(AssemblerDirectives, PUT_DirectiveOnly)
{
    createThisSourceFile(g_putFilename, " sta $ff\n");
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              1  put AssemblerTestPut\n",
                                                   "8000: 85 FF            1  sta $ff\n");
}

TEST(AssemblerDirectives, PUT_DirectiveAndContinueAsm)
{
    createThisSourceFile(g_putFilename, " sta $ff\n");
    m_pAssembler = Assembler_CreateFromString(dupe(" sta $7f\n"
                                                   " put AssemblerTestPut\n"
                                                   " sta $00\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8002: 85 FF            1  sta $ff\n",
                                                   "8004: 85 00        3  sta $00\n", 4);
}

TEST(AssemblerDirectives, PUT_DirectiveTwice)
{
    createThisSourceFile(g_putFilename, " sta $01\n");
    createThisSourceFile(g_putFilename2, " sta $02\n");
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut\n"
                                                   " put AssemblerTestPut2\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              2  put AssemblerTestPut2\n",
                                                   "8002: 85 02            1  sta $02\n", 4);

    LineInfo* pSecondLine = m_pAssembler->linesHead.pNext->pNext;
    LineInfo* pFourthLine = pSecondLine->pNext->pNext;
    LONGS_EQUAL(2, pSecondLine->machineCodeSize);
    LONGS_EQUAL(0, memcmp(pSecondLine->pMachineCode, "\x85\x01", 2));
    LONGS_EQUAL(2, pFourthLine->machineCodeSize);
    LONGS_EQUAL(0, memcmp(pFourthLine->pMachineCode, "\x85\x02", 2));
}

TEST(AssemblerDirectives, PUT_DirectiveWithPutDirsSetToCurrentDirectory)
{
    createThisSourceFile(g_putFilename, " sta $ff\n");
    m_initParams.pPutDirectories = ".";
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut\n"), &m_initParams);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              1  put AssemblerTestPut\n",
                                                   "8000: 85 FF            1  sta $ff\n");
}

TEST(AssemblerDirectives, PUT_DirectiveWithPutDirsSetToCurrentDirectoryWithTrailingSlash)
{
    createThisSourceFile(g_putFilename, " sta $ff\n");
    m_initParams.pPutDirectories = "./";
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut\n"), &m_initParams);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              1  put AssemblerTestPut\n",
                                                   "8000: 85 FF            1  sta $ff\n");
}

TEST(AssemblerDirectives, PUT_DirectiveWithPutDirsSetToInvalidDirectory)
{
    createThisSourceFile(g_putFilename, " sta $ff\n");
    m_initParams.pPutDirectories = "foo";
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut\n"), &m_initParams);
    runAssemblerAndValidateFailure("filename:1: error: Failed to PUT 'AssemblerTestPut.S' source file.\n", 
                                   "    :              1  put AssemblerTestPut\n");
}

TEST(AssemblerDirectives, PUT_DirectiveWithPutDirsSetToTwoComponentsFirstValidSecondInvalid)
{
    createThisSourceFile(g_putFilename, " sta $ff\n");
    m_initParams.pPutDirectories = ".;foo";
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut\n"), &m_initParams);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              1  put AssemblerTestPut\n",
                                                   "8000: 85 FF            1  sta $ff\n");
}

TEST(AssemblerDirectives, PUT_DirectiveWithPutDirsSetToTwoComponentsFirstInvalidSecondValid)
{
    createThisSourceFile(g_putFilename, " sta $ff\n");
    m_initParams.pPutDirectories = "foo;.";
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut\n"), &m_initParams);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              1  put AssemblerTestPut\n",
                                                   "8000: 85 FF            1  sta $ff\n");
}

TEST(AssemblerDirectives, PUT_DirectiveInvalidNesting)
{
    createThisSourceFile(g_putFilename, " put AssemblerTestPut2\n");
    createThisSourceFile(g_putFilename2, " sta $02\n");
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut\n"), NULL);
    runAssemblerAndValidateFailure("AssemblerTestPut.S:1: error: Can't nest PUT directive within another PUT file.\n", 
                                   "    :                  1  put AssemblerTestPut2\n", 3);
}

TEST(AssemblerDirectives, PUT_DirectiveWithErrorInPutFile)
{
    createThisSourceFile(g_putFilename, " foo\n");
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut\n"), NULL);
    runAssemblerAndValidateFailure("AssemblerTestPut.S:1: error: 'foo' is not a recognized mnemonic or macro.\n", 
                                   "    :                  1  foo\n", 3);
}

TEST(AssemblerDirectives, PUT_DirectiveInvalidForwardReferenceInPutFile)
{
    createThisSourceFile(g_putFilename, " sta Label\n");
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut\n"
                                                   "Label EQU $00\n"), NULL);
    runAssemblerAndValidateFailure("AssemblerTestPut.S:1: error: Couldn't properly infer size of a forward reference in 'Label' operand.\n", 
                                   "    :    =0000     2 Label EQU $00\n", 4);
}

TEST(AssemblerDirectives, PUT_DirectiveFailFileOpen)
{
    createThisSourceFile(g_putFilename, " sta $ff\n");
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut\n"), NULL);
    fopenFail(NULL);
        runAssemblerAndValidateFailure("filename:1: error: Failed to PUT 'AssemblerTestPut.S' source file.\n", 
                                       "    :              1  put AssemblerTestPut\n");
}

TEST(AssemblerDirectives, PUT_DirectiveFailAllAllocations)
{
    static const int allocationsToFail = 5;
    createThisSourceFile(g_putFilename, " sta $ff\n");
    for (int i = 3 ; i <= allocationsToFail ; i++)
    {
        m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut\n"), NULL);
        MallocFailureInject_FailAllocation(i);
        runAssemblerAndValidateFailure("filename:1: error: Failed to PUT 'AssemblerTestPut.S' source file.\n", 
                                       "    :              1  put AssemblerTestPut\n");
        MallocFailureInject_Restore();
        Assembler_Free(m_pAssembler);
        m_pAssembler = NULL;
        printfSpy_Unhook();
        printfSpy_Hook(128);
    }

    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut\n"), NULL);
    MallocFailureInject_FailAllocation(allocationsToFail + 1);
    __try_and_catch( Assembler_Run(m_pAssembler) );
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}

TEST(AssemblerDirectives, USR_DirectiveWithDirectorAndSuffixToRemoveFromSourceFilename)
{
    createSourceFile(" org $800\n"
                     " hex 00,ff\n"
                     " usr $a9,1,$a80,*-$800\n");
    m_pAssembler = Assembler_CreateFromFile("./AssemblerTest.S", NULL);
    runAssemblerAndValidateLastLineIs("    :              3  usr $a9,1,$a80,*-$800\n", 3);
    validateRW18ObjectFileContains(g_usrFilename, 0xa9, 1, 0xa80, "\x00\xff", 2);
}

TEST(AssemblerDirectives, USR_DirectiveWithSuffixToRemoveFromSourceFilename)
{
    createSourceFile(" org $800\n"
                     " hex 00,ff\n"
                     " usr $a9,1,$a80,*-$800\n");
    m_pAssembler = Assembler_CreateFromFile("AssemblerTest.S", NULL);
    runAssemblerAndValidateLastLineIs("    :              3  usr $a9,1,$a80,*-$800\n", 3);
    validateRW18ObjectFileContains(g_usrFilename, 0xa9, 1, 0xa80, "\x00\xff", 2);
}

TEST(AssemblerDirectives, USR_DirectiveWithNothingToRemoveFromSourceFilename)
{
    createThisSourceFile("AssemblerTest",
                         " org $800\n"
                         " hex 00,ff\n"
                         " usr $a9,1,$a80,*-$800\n");
    m_pAssembler = Assembler_CreateFromFile("AssemblerTest", NULL);
    runAssemblerAndValidateLastLineIs("    :              3  usr $a9,1,$a80,*-$800\n", 3);
    validateRW18ObjectFileContains(g_usrFilename, 0xa9, 1, 0xa80, "\x00\xff", 2);
}

TEST(AssemblerDirectives, USR_DirectiveWithDirectoryAndNoSuffixInSourceFilename)
{
    createThisSourceFile("AssemblerTest",
                         " org $800\n"
                         " hex 00,ff\n"
                         " usr $a9,1,$a80,*-$800\n");
    m_pAssembler = Assembler_CreateFromFile("./AssemblerTest", NULL);
    runAssemblerAndValidateLastLineIs("    :              3  usr $a9,1,$a80,*-$800\n", 3);
    validateRW18ObjectFileContains(g_usrFilename, 0xa9, 1, 0xa80, "\x00\xff", 2);
}

TEST(AssemblerDirectives, USR_DirectiveFailsWhenLessThan4ArgumentsInOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " hex 00,ff\n"
                                                   " usr $a9,1,$a80\n"), NULL);
    runAssemblerAndValidateFailure("filename:3: error: '$a9,1,$a80' doesn't contain the 4 arguments required for USR directive.\n", 
                                   "    :              3  usr $a9,1,$a80\n", 4);
}

TEST(AssemblerDirectives, USR_DirectiveFailsWhenMoreThan4ArgumentsInOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " hex 00,ff\n"
                                                   " usr $a9,1,$a80,*-$800,5\n"), NULL);
    runAssemblerAndValidateFailure("filename:3: error: '$a9,1,$a80,*-$800,5' doesn't contain the 4 arguments required for USR directive.\n", 
                                   "    :              3  usr $a9,1,$a80,*-$800,5\n", 4);
}

TEST(AssemblerDirectives, USR_DirectiveFailsBinaryBufferWriteQueueingOperation)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " hex 00,ff\n"
                                                   " usr $a9,1,$a80,*-$800\n"), NULL);
    MallocFailureInject_FailAllocation(4);
    __try_and_catch( Assembler_Run(m_pAssembler) );
    validateFailureOutput("filename:3: error: Failed to queue up USR save to 'filename.usr'.\n", 
                          "    :              3  usr $a9,1,$a80,*-$800\n", 4);
}

TEST(AssemblerDirectives, DO_DirectiveWithOneExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1\n"
                                                   " hex 00\n"
                                                   " fin\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 00           2  hex 00\n",
                                                   "    :              3  fin\n", 3);
}

TEST(AssemblerDirectives, DO_DirectiveWithMaxShortExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do $ffff\n"
                                                   " hex 00\n"
                                                   " fin\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 00           2  hex 00\n",
                                                   "    :              3  fin\n", 3);
}

TEST(AssemblerDirectives, DO_DirectiveWithZeroExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 0\n"
                                                   " hex 00\n"
                                                   " fin\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              2  hex 00\n",
                                                   "    :              3  fin\n", 3);
}

TEST(AssemblerDirectives, DO_DirectivewithZeroExpressionButContinueToAssembleAfterFIN)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 0\n"
                                                   " hex 00\n"
                                                   " fin\n"
                                                   " hex 01\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              3  fin\n",
                                                   "8000: 01           4  hex 01\n", 4);
}

TEST(AssemblerDirectives, DO_DirectiveWithDivExpressionThatResultsInNonZero)
{
    m_pAssembler = Assembler_CreateFromString(dupe("two equ 2\n"
                                                   " do 2/two\n"
                                                   " hex 00\n"
                                                   " fin\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 00           3  hex 00\n",
                                                   "    :              4  fin\n", 4);
}

TEST(AssemblerDirectives, DO_DirectiveWithDivExpressionThatResultsInZero)
{
    m_pAssembler = Assembler_CreateFromString(dupe("two equ 2\n"
                                                   " do 1/two\n"
                                                   " hex 00\n"
                                                   " fin\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              3  hex 00\n",
                                                   "    :              4  fin\n", 4);
}

TEST(AssemblerDirectives, DO_DirectiveWith0Else)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 0\n"
                                                   " hex 00\n"
                                                   " else\n"
                                                   " hex 01\n"
                                                   " fin\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 01           4  hex 01\n",
                                                   "    :              5  fin\n", 5);
}

TEST(AssemblerDirectives, DO_DirectiveWith1Else)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1\n"
                                                   " hex 00\n"
                                                   " else\n"
                                                   " hex 01\n"
                                                   " fin\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              4  hex 01\n",
                                                   "    :              5  fin\n", 5);
}

TEST(AssemblerDirectives, DO_DirectiveWith2LevelNestingTest00)
{
    m_pAssembler = Assembler_CreateFromString(dupe("test1 equ 0\n"
                                                   "test2 equ 0\n"
                                                   " do test1\n"
                                                   " do test2\n"
                                                   "label equ 1\n"
                                                   " else\n"
                                                   "label equ 2\n"
                                                   " fin\n"
                                                   " else\n"
                                                   " do test2\n"
                                                   "label equ 3\n"
                                                   " else\n"
                                                   "label equ 4\n"
                                                   " fin\n"
                                                   " fin\n"
                                                   " db label\n"), NULL);
    runAssemblerAndValidateLastLineIs("8000: 04          16  db label\n", 16);
}

TEST(AssemblerDirectives, DO_DirectiveWith2LevelNestingTest01)
{
    m_pAssembler = Assembler_CreateFromString(dupe("test1 equ 0\n"
                                                   "test2 equ 1\n"
                                                   " do test1\n"
                                                   " do test2\n"
                                                   "label equ 1\n"
                                                   " else\n"
                                                   "label equ 2\n"
                                                   " fin\n"
                                                   " else\n"
                                                   " do test2\n"
                                                   "label equ 3\n"
                                                   " else\n"
                                                   "label equ 4\n"
                                                   " fin\n"
                                                   " fin\n"
                                                   " db label\n"), NULL);
    runAssemblerAndValidateLastLineIs("8000: 03          16  db label\n", 16);
}

TEST(AssemblerDirectives, DO_DirectiveWith2LevelNestingTest10)
{
    m_pAssembler = Assembler_CreateFromString(dupe("test1 equ 1\n"
                                                   "test2 equ 0\n"
                                                   " do test1\n"
                                                   " do test2\n"
                                                   "label equ 1\n"
                                                   " else\n"
                                                   "label equ 2\n"
                                                   " fin\n"
                                                   " else\n"
                                                   " do test2\n"
                                                   "label equ 3\n"
                                                   " else\n"
                                                   "label equ 4\n"
                                                   " fin\n"
                                                   " fin\n"
                                                   " db label\n"), NULL);
    runAssemblerAndValidateLastLineIs("8000: 02          16  db label\n", 16);
}

TEST(AssemblerDirectives, DO_DirectiveWith2LevelNestingTest11)
{
    m_pAssembler = Assembler_CreateFromString(dupe("test1 equ 1\n"
                                                   "test2 equ 1\n"
                                                   " do test1\n"
                                                   " do test2\n"
                                                   "label equ 1\n"
                                                   " else\n"
                                                   "label equ 2\n"
                                                   " fin\n"
                                                   " else\n"
                                                   " do test2\n"
                                                   "label equ 3\n"
                                                   " else\n"
                                                   "label equ 4\n"
                                                   " fin\n"
                                                   " fin\n"
                                                   " db label\n"), NULL);
    runAssemblerAndValidateLastLineIs("8000: 01          16  db label\n", 16);
}

TEST(AssemblerDirectives, DO_DirectiveWith8LevelsOfNesting)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1\n"
                                                   " do 1\n"
                                                   " do 1\n"
                                                   " do 1\n"
                                                   " do 1\n"
                                                   " do 1\n"
                                                   " do 1\n"
                                                   " do 1\n"
                                                   "label equ $ff\n"
                                                   " fin\n"
                                                   " fin\n"
                                                   " fin\n"
                                                   " fin\n"
                                                   " fin\n"
                                                   " fin\n"
                                                   " fin\n"
                                                   " fin\n"
                                                   " db label\n"), NULL);
    runAssemblerAndValidateLastLineIs("8000: FF          18  db label\n", 18);
}

TEST(AssemblerDirectives, DO_DirectiveWithForwardReferenceInClause)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1\n"
                                                   " db label\n"
                                                   " fin\n"
                                                   "label equ $ff"), NULL);
    Assembler_Run(m_pAssembler);

    LineInfo* pSecondLine = m_pAssembler->linesHead.pNext->pNext;
    LONGS_EQUAL(1, pSecondLine->machineCodeSize);
    LONGS_EQUAL(0, memcmp(pSecondLine->pMachineCode, "\xff", 1));
}

TEST(AssemblerDirectives, DO_DirectiveWithNoExpressionIsInvalid)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: do directive requires operand.\n", 
                                    "    :              1  do\n", 2);
}

TEST(AssemblerDirectives, DO_DirectiveWithForwardExpressionIsInvalid)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do ForwardLabel\n"
                                                   "ForwardLabel equ 1\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: do directive can't forward reference labels.\n", 
                                   "    :    =0001     2 ForwardLabel equ 1\n", 3);
}

TEST(AssemblerDirectives, ELSE_DirectiveWithExpressionIsInvalid)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1\n"
                                                   " hex 00\n"
                                                   " else 0\n"
                                                   " hex 01\n"
                                                   " fin\n"), NULL);
    runAssemblerAndValidateFailure("filename:3: error: else directive doesn't require operand.\n", 
                                   "    :              5  fin\n", 6);
}

TEST(AssemblerDirectives, FIN_DirectiveWithExpressionIsInvalid)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1\n"
                                                   " hex 00\n"
                                                   " else\n"
                                                   " hex 01\n"
                                                   " fin 1\n"
                                                   " fin\n"), NULL);
    runAssemblerAndValidateFailure("filename:5: error: fin directive doesn't require operand.\n", 
                                    "    :              6  fin\n", 7);
}

TEST(AssemblerDirectives, DO_DirectiveWithNoMatchingFIN)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1\n"
                                                   " do 1\n"
                                                   " fin\n"), NULL);
    runAssemblerAndValidateFailure("filename:3: error: DO/IF directive is missing matching FIN directive.\n", 
                                   "    :              3  fin\n", 4);
}

TEST(AssemblerDirectives, FIN_DirectiveWithNoMatchingDO)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1\n"
                                                   " fin\n"
                                                   " fin\n"), NULL);
    runAssemblerAndValidateFailure("filename:3: error: fin directive without corresponding DO/IF directive.\n", 
                                   "    :              3  fin\n", 4);
}

TEST(AssemblerDirectives, ELSE_DirectiveWithNoMatchingDO)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1\n"
                                                   " fin\n"
                                                   " else\n"), NULL);
    runAssemblerAndValidateFailure("filename:3: error: else directive without corresponding DO/IF directive.\n", 
                                   "    :              3  else\n", 4);
}

TEST(AssemblerDirectives, ELSE_DirectiveWhenELSEAlreadyUsed)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1\n"
                                                   " else\n"
                                                   " else\n"
                                                   " fin\n"), NULL);
    runAssemblerAndValidateFailure("filename:3: error: Can't have multiple ELSE directives in a DO/IF clause.\n", 
                                   "    :              4  fin\n", 5);
}

TEST(AssemblerDirectives, DO_DirectiveWithFailedAllocationForConditional)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1\n"), NULL);
    MallocFailureInject_FailAllocation(2);
    __try_and_catch( Assembler_Run(m_pAssembler) );
    validateFailureOutput("filename:1: error: Failed to allocate space for DO conditional storage.\n", 
                          "    :              1  do 1\n", 2);
}


