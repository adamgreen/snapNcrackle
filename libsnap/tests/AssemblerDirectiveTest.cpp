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
/* Tests for the assembler directives.
   NOTE: EQU directive is tested as part of the AssemblerLabelTest set of unit tests.
*/
#include "AssemblerBaseTest.h"


static const char* g_putFilename = "AssemblerTestPut.S";
static const char* g_putFilename2 = "AssemblerTestPut2.S";
static const char* g_usrFilename = "AssemblerTest";


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
    m_pAssembler = Assembler_CreateFromString(dupe(" lst off" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("    :              1  lst off" LINE_ENDING);
}

TEST(AssemblerDirectives, IgnoreLSTDODirective)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lstdo off" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("    :              1  lstdo off" LINE_ENDING);
}

TEST(AssemblerDirectives, HEXDirectiveWithSingleValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 01" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: 01           1  hex 01" LINE_ENDING);
}

TEST(AssemblerDirectives, HEXDirectiveWithMixedCase)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex cD,Cd" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: CD CD        1  hex cD,Cd" LINE_ENDING);
}

TEST(AssemblerDirectives, HEXDirectiveWithSingleValueAndFollowedByComment)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 01 ; Comment" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: 01           1  hex 01 ; Comment" LINE_ENDING);
}

TEST(AssemblerDirectives, HEXDirectiveWithThreeValuesAndCommas)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 0e,0c,0a" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: 0E 0C 0A     1  hex 0e,0c,0a" LINE_ENDING);
}

TEST(AssemblerDirectives, HEXDirectiveWithThreeValues)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 0e0c0a" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: 0E 0C 0A     1  hex 0e0c0a" LINE_ENDING);
}

TEST(AssemblerDirectives, HEXDirectiveWithFourValues)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 01,02,03,04" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 01 02 03     1  hex 01,02,03,04" LINE_ENDING,
                                                   "8003: 04      " LINE_ENDING);
}

TEST(AssemblerDirectives, HEXDirectiveWithSixValues)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 01,02,03,04,05,06" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 01 02 03     1  hex 01,02,03,04,05,06" LINE_ENDING,
                                                   "8003: 04 05 06" LINE_ENDING);
}

TEST(AssemblerDirectives, HEXDirectiveWithMaximumOf32Value)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20" LINE_ENDING),
                                              NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("801B: 1C 1D 1E" LINE_ENDING,
                                                   "801E: 1F 20   " LINE_ENDING, 11);
}

TEST(AssemblerDirectives, HEXDirectiveWith33Values_1MoreThanSupported)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f2021" LINE_ENDING),
                                              NULL);
    runAssemblerAndValidateFailure("filename:1: error: '0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f2021' contains more than 32 values." LINE_ENDING, 
                                   "801E: 1F 20   " LINE_ENDING, 12);
}

TEST(AssemblerDirectives, HEXDirectiveOnTwoLines)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex 01" LINE_ENDING
                                                   " hex 02" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 01           1  hex 01" LINE_ENDING,
                                                   "8001: 02           2  hex 02" LINE_ENDING);
}

TEST(AssemblerDirectives, HEXDirectiveWithUpperCaseHex)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex FA" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: FA           1  hex FA" LINE_ENDING);
}

TEST(AssemblerDirectives, HEXDirectiveWithLowerCaseHex)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex fa" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: FA           1  hex fa" LINE_ENDING);
}

TEST(AssemblerDirectives, HEXDirectiveWithOddDigitCount)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex fa0" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: 'fa0' doesn't contain an even number of hex digits." LINE_ENDING,
                                   "    :              1  hex fa0" LINE_ENDING);
}

TEST(AssemblerDirectives, HEXDirectiveWithInvalidDigit)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex fg" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: 'fg' contains an invalid hex digit." LINE_ENDING,
                                   "    :              1  hex fg" LINE_ENDING);
}

TEST(AssemblerDirectives, HEXDirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: hex directive requires operand." LINE_ENDING,
                                   "    :              1  hex" LINE_ENDING, 2);
}

TEST(AssemblerDirectives, FailBinaryBufferAllocationInHEXDirective)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" hex ff" LINE_ENDING), NULL);
    BinaryBuffer_FailAllocation(m_pAssembler->pCurrentBuffer, 1);
    runAssemblerAndValidateFailure("filename:1: error: Exceeded the 65536 allowed bytes in the object file." LINE_ENDING,
                                   "    :              1  hex ff" LINE_ENDING);
}

TEST(AssemblerDirectives, ORGDirectiveWithLiteralValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $900" LINE_ENDING
                                                   " hex 01" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              1  org $900" LINE_ENDING, 
                                                   "0900: 01           2  hex 01" LINE_ENDING);
}

TEST(AssemblerDirectives, ORGDirectiveWithInvalidExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org +900" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '+900' expression." LINE_ENDING,
                                   "    :              1  org +900" LINE_ENDING);
}

TEST(AssemblerDirectives, ORGDirectiveWithSymbolValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe("org = $800" LINE_ENDING
                                                   " org org" LINE_ENDING
                                                   " hex 01" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              2  org org" LINE_ENDING,
                                                   "0800: 01           3  hex 01" LINE_ENDING, 3);
}

TEST(AssemblerDirectives, ORGDirectiveWithInvalidImmediate)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org #$00" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: '#$00' doesn't specify an absolute address." LINE_ENDING,
                                   "    :              1  org #$00" LINE_ENDING);
}

TEST(AssemblerDirectives, ORGDirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: org directive requires operand." LINE_ENDING,
                                   "    :              1  org" LINE_ENDING, 2);
}

TEST(AssemblerDirectives, DUMandDEND_Directive)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800" LINE_ENDING
                                                   " dum $00" LINE_ENDING
                                                   " hex ff" LINE_ENDING
                                                   " dend" LINE_ENDING
                                                   " hex fe" LINE_ENDING), NULL);
    Assembler_Run(m_pAssembler);
    
    LineInfo* pThirdLine = m_pAssembler->linesHead.pNext->pNext->pNext;
    LineInfo* pFifthLine = pThirdLine->pNext->pNext;
    
    validateLineInfo(pThirdLine, 0x0000, 1, "\xff");
    validateLineInfo(pFifthLine, 0x0800, 1, "\xfe");
}

TEST(AssemblerDirectives, TwoDUMandDEND_Directive)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800" LINE_ENDING
                                                   " dum $00" LINE_ENDING
                                                   " hex ff" LINE_ENDING
                                                   " dum $100" LINE_ENDING
                                                   " hex fe" LINE_ENDING
                                                   " dend" LINE_ENDING
                                                   " hex fd" LINE_ENDING), NULL);
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
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800" LINE_ENDING
                                                   " dum $00" LINE_ENDING
                                                   " hex ff" LINE_ENDING
                                                   " org $100" LINE_ENDING
                                                   " hex fe" LINE_ENDING
                                                   " dend" LINE_ENDING
                                                   " hex fd" LINE_ENDING), NULL);
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
    m_pAssembler = Assembler_CreateFromString(dupe(" dum #$00" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: '#$00' doesn't specify an absolute address." LINE_ENDING,
                                   "    :              1  dum #$00" LINE_ENDING);
}

TEST(AssemblerDirectives, DEND_DirectiveWithoutDUM)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dend" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: dend isn't allowed without a preceding DUM directive." LINE_ENDING,
                                   "    :              1  dend" LINE_ENDING);
}

TEST(AssemblerDirectives, DUMDirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dum" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: dum directive requires operand." LINE_ENDING,
                                   "    :              1  dum" LINE_ENDING, 2);
}

TEST(AssemblerDirectives, DENDDirectiveWithOperandWhenNotExpected)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dum 0" LINE_ENDING
                                                   " dend $100" LINE_ENDING), NULL);
    runAssemblerAndValidateWarning("filename:2: warning: dend directive ignoring operand as comment." LINE_ENDING,
                                   "    :              2  dend $100" LINE_ENDING, 3);
}

TEST(AssemblerDirectives, DS_DirectiveWithSmallRepeatValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds 1" LINE_ENDING
                                                   " hex ff" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 00           1  ds 1" LINE_ENDING, 
                                                   "8001: FF           2  hex ff" LINE_ENDING);
}

TEST(AssemblerDirectives, DS_DirectiveWithRepeatValueGreaterThan32)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds 42" LINE_ENDING
                                                   " hex ff" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8027: 00 00 00" LINE_ENDING, 
                                                   "802A: FF           2  hex ff" LINE_ENDING, 15);
}

TEST(AssemblerDirectives, DS_DirectiveWithBackSlashWhenAlreadyOnPageBoundary)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds \\" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("    :              1  ds \\" LINE_ENDING);
}

TEST(AssemblerDirectives, DS_DirectiveWithBackSlashWhenOneByteFromPageBoundary)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds 255" LINE_ENDING
                                                   " ds \\,$ff" LINE_ENDING), NULL);
    runAssemblerAndValidateLastLineIs("80FF: FF           2  ds \\,$ff" LINE_ENDING, 86);
}

TEST(AssemblerDirectives, DS_DirectiveWithSecondExpressionToSpecifyFillValue)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds 2,$ff" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: FF FF        1  ds 2,$ff" LINE_ENDING);
}

TEST(AssemblerDirectives, DS_DirectiveWithInvalidExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds ($800" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '($800' expression." LINE_ENDING,
                                   "    :              1  ds ($800" LINE_ENDING);
}

TEST(AssemblerDirectives, DS_DirectiveWithForwardReference)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds \\,Fill" LINE_ENDING
                                                   "Fill equ $ff" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              1  ds \\,Fill" LINE_ENDING,
                                                   "    :    =00FF     2 Fill equ $ff" LINE_ENDING);
}

TEST(AssemblerDirectives, DS_DirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: ds directive requires operand." LINE_ENDING,
                                   "    :              1  ds" LINE_ENDING, 2);
}

TEST(AssemblerDirectives, FailBinaryBufferAllocationInDSDirective)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" ds 1" LINE_ENDING), NULL);
    BinaryBuffer_FailAllocation(m_pAssembler->pCurrentBuffer, 1);
    runAssemblerAndValidateFailure("filename:1: error: Exceeded the 65536 allowed bytes in the object file." LINE_ENDING,
                                   "    :              1  ds 1" LINE_ENDING);
}

TEST(AssemblerDirectives, ASC_DirectiveInDoubleQuotes)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" asc \"Tst\"" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: D4 F3 F4     1  asc \"Tst\"" LINE_ENDING);
}

TEST(AssemblerDirectives, ASC_DirectiveInDoubleQuotesAndFollowedByComment)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" asc \"Tst\" ;Comment" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: D4 F3 F4     1  asc \"Tst\" ;Comment" LINE_ENDING);
}

TEST(AssemblerDirectives, ASC_DirectiveInSingleQuotes)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" asc 'Tst'" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: 54 73 74     1  asc 'Tst'" LINE_ENDING);
}

TEST(AssemblerDirectives, ASC_DirectiveWithSpaceBetweenQuotes)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" asc 'a b'" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: 61 20 62     1  asc 'a b'" LINE_ENDING);
}

TEST(AssemblerDirectives, ASC_DirectiveWithHexSuffix)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" asc 'T',00ff" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: 54 00 FF     1  asc 'T',00ff" LINE_ENDING);
}

TEST(AssemblerDirectives, ASC_DirectiveWithHexSuffixContainingCommas)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" asc 'T',00,FF" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: 54 00 FF     1  asc 'T',00,FF" LINE_ENDING);
}


TEST(AssemblerDirectives, ASC_DirectiveWithNoEndingDelimiter)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" asc 'Tst" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: 'Tst didn't end with the expected ' delimiter." LINE_ENDING,
                                   "8000: 54 73 74     1  asc 'Tst" LINE_ENDING);
}

TEST(AssemblerDirectives, ASC_DirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" asc" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: asc directive requires operand." LINE_ENDING,
                                   "    :              1  asc" LINE_ENDING, 2);
}

TEST(AssemblerDirectives, FailBinaryBufferAllocationInASCDirective)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" asc 'Tst'" LINE_ENDING), NULL);
    BinaryBuffer_FailAllocation(m_pAssembler->pCurrentBuffer, 1);
    runAssemblerAndValidateFailure("filename:1: error: Exceeded the 65536 allowed bytes in the object file." LINE_ENDING,
                                   "    :              1  asc 'Tst'" LINE_ENDING);
}

TEST(AssemblerDirectives, REV_DirectiveInDoubleQuotes)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" rev \"Tst\"" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: F4 F3 D4     1  rev \"Tst\"" LINE_ENDING);
}

TEST(AssemblerDirectives, REV_DirectiveInSingleQuotes)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" rev 'Tst'" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: 74 73 54     1  rev 'Tst'" LINE_ENDING);
}

TEST(AssemblerDirectives, REV_DirectiveWithComment)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" rev 'Tst' Comment" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: 74 73 54     1  rev 'Tst' Comment" LINE_ENDING);
}

TEST(AssemblerDirectives, REV_DirectiveWithNoEndingDelimiter)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" rev 'Tst" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: 'Tst didn't end with the expected ' delimiter." LINE_ENDING,
                                   "8000: 74 73 54     1  rev 'Tst" LINE_ENDING);
}

TEST(AssemblerDirectives, REV_DirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" rev" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: rev directive requires operand." LINE_ENDING,
                                   "    :              1  rev" LINE_ENDING, 2);
}

TEST(AssemblerDirectives, FailBinaryBufferAllocationInREVDirective)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" rev 'Tst'" LINE_ENDING), NULL);
    BinaryBuffer_FailAllocation(m_pAssembler->pCurrentBuffer, 1);
    runAssemblerAndValidateFailure("filename:1: error: Exceeded the 65536 allowed bytes in the object file." LINE_ENDING,
                                   "    :              1  rev 'Tst'" LINE_ENDING);
}

TEST(AssemblerDirectives, SAV_DirectiveOnEmptyObjectFile)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sav AssemblerTest.sav" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("    :              1  sav AssemblerTest.sav" LINE_ENDING);
    validateObjectFileContains(0x8000, "", 0);
}

TEST(AssemblerDirectives, SAV_DirectiveOnSmallObjectFile)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800" LINE_ENDING
                                                   " hex 00,ff" LINE_ENDING
                                                   " sav AssemblerTest.sav" LINE_ENDING), NULL);
    runAssemblerAndValidateLastLineIs("    :              3  sav AssemblerTest.sav" LINE_ENDING, 3);
    validateObjectFileContains(0x800, "\x00\xff", 2);
}

TEST(AssemblerDirectives, SAV_DirectiveOnSmallObjectFileOverridingOutputDirectoryWithoutSlash)
{
    m_initParams.pOutputDirectory = ".";
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800" LINE_ENDING
                                                   " hex 00,ff" LINE_ENDING
                                                   " sav AssemblerTest.sav" LINE_ENDING), &m_initParams);
    runAssemblerAndValidateLastLineIs("    :              3  sav AssemblerTest.sav" LINE_ENDING, 3);
    validateObjectFileContains(0x800, "\x00\xff", 2);
}

TEST(AssemblerDirectives, SAV_DirectiveOnSmallObjectFileOverridingOutputDirectoryWithSlash)
{
    m_initParams.pOutputDirectory = "." SLASH_STR;
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800" LINE_ENDING
                                                   " hex 00,ff" LINE_ENDING
                                                   " sav AssemblerTest.sav" LINE_ENDING), &m_initParams);
    runAssemblerAndValidateLastLineIs("    :              3  sav AssemblerTest.sav" LINE_ENDING, 3);
    validateObjectFileContains(0x800, "\x00\xff", 2);
}

TEST(AssemblerDirectives, SAV_DirectiveOnSmallObjectFileOverridingToInvalidOutputDirectory)
{
    m_initParams.pOutputDirectory = "foobar" SLASH_STR;
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800" LINE_ENDING
                                                   " hex 00,ff" LINE_ENDING
                                                   " sav AssemblerTest.sav" LINE_ENDING), &m_initParams);
    __try_and_catch( Assembler_Run(m_pAssembler) );
    STRCMP_EQUAL("filename:3: error: Failed to save output." LINE_ENDING, printfSpy_GetLastOutput());
    validateExceptionThrown(fileException);
}

TEST(AssemblerDirectives, SAV_DirectiveShouldBeIgnoredOnErrors)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800" LINE_ENDING
                                                   " hex 00,ff" LINE_ENDING
                                                   " hex ($00)" LINE_ENDING
                                                   " sav AssemblerTest.sav" LINE_ENDING), NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    m_pFile = fopen(g_objectFilename, "rb");
    POINTERS_EQUAL(NULL, m_pFile);
}

TEST(AssemblerDirectives, SAV_DirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sav" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: sav directive requires operand." LINE_ENDING,
                                   "    :              1  sav" LINE_ENDING, 2);
}

TEST(AssemblerDirectives, SAV_DirectiveFailWriteFileQueue)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sav AssemblerTest.sav" LINE_ENDING), NULL);
    MallocFailureInject_FailAllocation(2);
    runAssemblerAndValidateFailure("filename:1: error: Failed to queue up save to 'AssemblerTest.sav'." LINE_ENDING,
                                   "    :              1  sav AssemblerTest.sav" LINE_ENDING);
}

TEST(AssemblerDirectives, DB_DirectiveWithSingleExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe("Value EQU $fe" LINE_ENDING
                                                   " db Value+1" LINE_ENDING), NULL);
    runAssemblerAndValidateLastLineIs("8000: FF           2  db Value+1" LINE_ENDING, 2);
}

TEST(AssemblerDirectives, DB_DirectiveWithSingleExpressionAndComment)
{
    m_pAssembler = Assembler_CreateFromString(dupe("Value EQU $fe" LINE_ENDING
                                                   " db Value+1 ;Comment" LINE_ENDING), NULL);
    runAssemblerAndValidateLastLineIs("8000: FF           2  db Value+1 ;Comment" LINE_ENDING, 2);
}

TEST(AssemblerDirectives, DB_DirectiveWithThreeExpressions)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" db 2,0,1" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: 02 00 01     1  db 2,0,1" LINE_ENDING);
}

TEST(AssemblerDirectives, DB_DirectiveWithImmediateExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" db #$ff" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: FF           1  db #$ff" LINE_ENDING);
}

TEST(AssemblerDirectives, DB_DirectiveWithForwardReference)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" db Label" LINE_ENDING
                                                   "Label db $12" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 01           1  db Label" LINE_ENDING,
                                                   "8001: 12           2 Label db $12" LINE_ENDING);
}

TEST(AssemblerDirectives, DB_DirectiveWithInvalidExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" db ($800" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '($800' expression." LINE_ENDING,
                                   "    :              1  db ($800" LINE_ENDING);
}

TEST(AssemblerDirectives, DB_DirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" db" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: db directive requires operand." LINE_ENDING,
                                   "    :              1  db" LINE_ENDING, 2);
}

TEST(AssemblerDirectives, DFB_DirectiveSameAsDB)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dfb 2,0,1" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: 02 00 01     1  dfb 2,0,1" LINE_ENDING);
}

TEST(AssemblerDirectives, DFB_DirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dfb" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: dfb directive requires operand." LINE_ENDING,
                                   "    :              1  dfb" LINE_ENDING, 2);
}

TEST(AssemblerDirectives, TR_DirectiveIsIgnored)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" tr on" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("    :              1  tr on" LINE_ENDING);
}

TEST(AssemblerDirectives, DA_DirectiveWithOneExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" da $ff+1" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: 00 01        1  da $ff+1" LINE_ENDING);
}

TEST(AssemblerDirectives, DA_DirectiveWithOneExpressionAndComment)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" da $ff+1 ;Comment" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("8000: 00 01        1  da $ff+1 ;Comment" LINE_ENDING);
}

TEST(AssemblerDirectives, DA_DirectiveWithThreeExpressions)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" da $ff+1,$ff,$1233+1" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 00 01 FF     1  da $ff+1,$ff,$1233+1" LINE_ENDING,
                                                   "8003: 00 34 12" LINE_ENDING);
}

TEST(AssemblerDirectives, DA_DirectiveWithForwardReference)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" da Label" LINE_ENDING
                                                   "Label da $1234" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 02 80        1  da Label" LINE_ENDING,
                                                   "8002: 34 12        2 Label da $1234" LINE_ENDING);
}

TEST(AssemblerDirectives, DA_DirectiveWithInvalidExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" da ($800" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: Unexpected prefix in '($800' expression." LINE_ENDING,
                                   "    :              1  da ($800" LINE_ENDING);
}

TEST(AssemblerDirectives, DA_DirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" da" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: da directive requires operand." LINE_ENDING,
                                   "    :              1  da" LINE_ENDING, 2);
}

TEST(AssemblerDirectives, DW_DirectiveSameAsDA)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dw $ff+1,$ff,$1233+1" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 00 01 FF     1  dw $ff+1,$ff,$1233+1" LINE_ENDING,
                                                   "8003: 00 34 12" LINE_ENDING);
}

TEST(AssemblerDirectives, DW_DirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" dw" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: dw directive requires operand." LINE_ENDING,
                                   "    :              1  dw" LINE_ENDING, 2);
}

/* UNDONE: This should test that a 65C02 instruction is allowed after issue. */
TEST(AssemblerDirectives, XC_DirectiveSingleInvocation)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" xc" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("    :              1  xc" LINE_ENDING);
}

/* UNDONE: This is testing a temporary hack to ignore 65802/65816 instructions as I don't support those at this time. */
TEST(AssemblerDirectives, XC_DirectiveTwiceShouldCauseRTSInsertion)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" xc" LINE_ENDING
                                                   " xc" LINE_ENDING
                                                   " lda #20" LINE_ENDING), NULL);
    runAssemblerAndValidateLastLineIs("8000: 60           3  lda #20" LINE_ENDING, 3);
}

TEST(AssemblerDirectives, XC_DirectiveTwiceShouldCauseRTSInsertionForUnknownOpcodes)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" xc" LINE_ENDING
                                                   " xc" LINE_ENDING
                                                   " foobar" LINE_ENDING), NULL);
    runAssemblerAndValidateLastLineIs("8000: 60           3  foobar" LINE_ENDING, 3);
}

TEST(AssemblerDirectives, XC_DirectiveWithOffOperandShouldResetTo6502)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" xc" LINE_ENDING
                                                   " xc" LINE_ENDING
                                                   " xc off" LINE_ENDING
                                                   " lda #20" LINE_ENDING), NULL);
    runAssemblerAndValidateLastLineIs("8000: A9 14        4  lda #20" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, XC_DirectiveThriceShouldCauseError)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" xc" LINE_ENDING
                                                   " xc" LINE_ENDING
                                                   " xc" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:3: error: Can't have more than 2 XC directives." LINE_ENDING,
                                   "    :              3  xc" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, XC_DirectiveForwardReferenceFrom6502To65816)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lda #ForwardLabel" LINE_ENDING
                                                   " xc" LINE_ENDING
                                                   " xc" LINE_ENDING
                                                   "ForwardLabel lda #20" LINE_ENDING), NULL);
    runAssemblerAndValidateLastLineIs("8002: 60           4 ForwardLabel lda #20" LINE_ENDING, 4);
}

/* UNDONE: This should be supported in the future. */
TEST(AssemblerDirectives, MX_DirectiveIgnored)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" mx" LINE_ENDING), NULL);
    runAssemblerAndValidateOutputIs("    :              1  mx" LINE_ENDING);
}

TEST(AssemblerDirectives, PUT_DirectiveOnly)
{
    createThisSourceFile(g_putFilename, " sta $ff" LINE_ENDING);
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              1  put AssemblerTestPut" LINE_ENDING,
                                                   "8000: 85 FF            1  sta $ff" LINE_ENDING);
}

TEST(AssemblerDirectives, PUT_DirectiveAndContinueAsm)
{
    createThisSourceFile(g_putFilename, " sta $ff" LINE_ENDING);
    m_pAssembler = Assembler_CreateFromString(dupe(" sta $7f" LINE_ENDING
                                                   " put AssemblerTestPut" LINE_ENDING
                                                   " sta $00" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8002: 85 FF            1  sta $ff" LINE_ENDING,
                                                   "8004: 85 00        3  sta $00" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, PUT_DirectiveTwice)
{
    createThisSourceFile(g_putFilename, " sta $01" LINE_ENDING);
    createThisSourceFile(g_putFilename2, " sta $02" LINE_ENDING);
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut" LINE_ENDING
                                                   " put AssemblerTestPut2" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              2  put AssemblerTestPut2" LINE_ENDING,
                                                   "8002: 85 02            1  sta $02" LINE_ENDING, 4);

    LineInfo* pSecondLine = m_pAssembler->linesHead.pNext->pNext;
    LineInfo* pFourthLine = pSecondLine->pNext->pNext;
    LONGS_EQUAL(2, pSecondLine->machineCodeSize);
    LONGS_EQUAL(0, memcmp(pSecondLine->pMachineCode, "\x85\x01", 2));
    LONGS_EQUAL(2, pFourthLine->machineCodeSize);
    LONGS_EQUAL(0, memcmp(pFourthLine->pMachineCode, "\x85\x02", 2));
}

TEST(AssemblerDirectives, PUT_DirectiveWithPutDirsSetToCurrentDirectory)
{
    createThisSourceFile(g_putFilename, " sta $ff" LINE_ENDING);
    m_initParams.pPutDirectories = ".";
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut" LINE_ENDING), &m_initParams);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              1  put AssemblerTestPut" LINE_ENDING,
                                                   "8000: 85 FF            1  sta $ff" LINE_ENDING);
}

TEST(AssemblerDirectives, PUT_DirectiveWithPutDirsSetToCurrentDirectoryWithTrailingSlash)
{
    createThisSourceFile(g_putFilename, " sta $ff" LINE_ENDING);
    m_initParams.pPutDirectories = "." SLASH_STR;
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut" LINE_ENDING), &m_initParams);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              1  put AssemblerTestPut" LINE_ENDING,
                                                   "8000: 85 FF            1  sta $ff" LINE_ENDING);
}

TEST(AssemblerDirectives, PUT_DirectiveWithPutDirsSetToInvalidDirectory)
{
    createThisSourceFile(g_putFilename, " sta $ff" LINE_ENDING);
    m_initParams.pPutDirectories = "foo";
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut" LINE_ENDING), &m_initParams);
    runAssemblerAndValidateFailure("filename:1: error: Failed to PUT 'AssemblerTestPut.S' source file." LINE_ENDING, 
                                   "    :              1  put AssemblerTestPut" LINE_ENDING);
}

TEST(AssemblerDirectives, PUT_DirectiveWithPutDirsSetToTwoComponentsFirstValidSecondInvalid)
{
    createThisSourceFile(g_putFilename, " sta $ff" LINE_ENDING);
    m_initParams.pPutDirectories = ".;foo";
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut" LINE_ENDING), &m_initParams);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              1  put AssemblerTestPut" LINE_ENDING,
                                                   "8000: 85 FF            1  sta $ff" LINE_ENDING);
}

TEST(AssemblerDirectives, PUT_DirectiveWithPutDirsSetToTwoComponentsFirstInvalidSecondValid)
{
    createThisSourceFile(g_putFilename, " sta $ff" LINE_ENDING);
    m_initParams.pPutDirectories = "foo;.";
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut" LINE_ENDING), &m_initParams);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              1  put AssemblerTestPut" LINE_ENDING,
                                                   "8000: 85 FF            1  sta $ff" LINE_ENDING);
}

TEST(AssemblerDirectives, PUT_DirectiveInvalidNesting)
{
    createThisSourceFile(g_putFilename, " put AssemblerTestPut2" LINE_ENDING);
    createThisSourceFile(g_putFilename2, " sta $02" LINE_ENDING);
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("AssemblerTestPut.S:1: error: Can't nest PUT directive within another PUT file." LINE_ENDING, 
                                   "    :                  1  put AssemblerTestPut2" LINE_ENDING, 3);
}

TEST(AssemblerDirectives, PUT_DirectiveWithErrorInPutFile)
{
    createThisSourceFile(g_putFilename, " foo" LINE_ENDING);
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("AssemblerTestPut.S:1: error: 'foo' is not a recognized mnemonic or macro." LINE_ENDING, 
                                   "    :                  1  foo" LINE_ENDING, 3);
}

TEST(AssemblerDirectives, PUT_DirectiveInvalidForwardReferenceInPutFile)
{
    createThisSourceFile(g_putFilename, " sta Label" LINE_ENDING);
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut" LINE_ENDING
                                                   "Label EQU $00" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("AssemblerTestPut.S:1: error: Couldn't properly infer size of a forward reference in 'Label' operand." LINE_ENDING, 
                                   "    :    =0000     2 Label EQU $00" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, PUT_DirectiveFailFileOpen)
{
    createThisSourceFile(g_putFilename, " sta $ff" LINE_ENDING);
    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut" LINE_ENDING), NULL);
    fopenFail(NULL);
        runAssemblerAndValidateFailure("filename:1: error: Failed to PUT 'AssemblerTestPut.S' source file." LINE_ENDING, 
                                       "    :              1  put AssemblerTestPut" LINE_ENDING);
}

TEST(AssemblerDirectives, PUT_DirectiveFailAllAllocations)
{
    static const int allocationsToFail = 5;
    createThisSourceFile(g_putFilename, " sta $ff" LINE_ENDING);
    for (int i = 3 ; i <= allocationsToFail ; i++)
    {
        m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut" LINE_ENDING), NULL);
        MallocFailureInject_FailAllocation(i);
        runAssemblerAndValidateFailure("filename:1: error: Failed to PUT 'AssemblerTestPut.S' source file." LINE_ENDING, 
                                       "    :              1  put AssemblerTestPut" LINE_ENDING);
        MallocFailureInject_Restore();
        Assembler_Free(m_pAssembler);
        m_pAssembler = NULL;
        printfSpy_Unhook();
        printfSpy_Hook(128);
    }

    m_pAssembler = Assembler_CreateFromString(dupe(" put AssemblerTestPut" LINE_ENDING), NULL);
    MallocFailureInject_FailAllocation(allocationsToFail + 1);
    __try_and_catch( Assembler_Run(m_pAssembler) );
    LONGS_EQUAL(outOfMemoryException, getExceptionCode());
    clearExceptionCode();
}

TEST(AssemblerDirectives, USR_DirectiveWithDirectoryAndSuffixToRemoveFromSourceFilename)
{
    createSourceFile(" org $800" LINE_ENDING
                     " hex 00,ff" LINE_ENDING
                     " usr $a9,1,$a80,*-$800" LINE_ENDING);
    m_pAssembler = Assembler_CreateFromFile("." SLASH_STR "AssemblerTest.S", NULL);
    runAssemblerAndValidateLastLineIs("    :              3  usr $a9,1,$a80,*-$800" LINE_ENDING, 3);
    validateRW18ObjectFileContains(g_usrFilename, 0xa9, 1, 0xa80, "\x00\xff", 2);
}

TEST(AssemblerDirectives, USR_DirectiveWithSuffixToRemoveFromSourceFilename)
{
    createSourceFile(" org $800" LINE_ENDING
                     " hex 00,ff" LINE_ENDING
                     " usr $a9,1,$a80,*-$800" LINE_ENDING);
    m_pAssembler = Assembler_CreateFromFile("AssemblerTest.S", NULL);
    runAssemblerAndValidateLastLineIs("    :              3  usr $a9,1,$a80,*-$800" LINE_ENDING, 3);
    validateRW18ObjectFileContains(g_usrFilename, 0xa9, 1, 0xa80, "\x00\xff", 2);
}

TEST(AssemblerDirectives, USR_DirectiveWithNothingToRemoveFromSourceFilename)
{
    createThisSourceFile("AssemblerTest",
                         " org $800" LINE_ENDING
                         " hex 00,ff" LINE_ENDING
                         " usr $a9,1,$a80,*-$800" LINE_ENDING);
    m_pAssembler = Assembler_CreateFromFile("AssemblerTest", NULL);
    runAssemblerAndValidateLastLineIs("    :              3  usr $a9,1,$a80,*-$800" LINE_ENDING, 3);
    validateRW18ObjectFileContains(g_usrFilename, 0xa9, 1, 0xa80, "\x00\xff", 2);
}

TEST(AssemblerDirectives, USR_DirectiveWithDirectoryAndNoSuffixInSourceFilename)
{
    createThisSourceFile("AssemblerTest",
                         " org $800" LINE_ENDING
                         " hex 00,ff" LINE_ENDING
                         " usr $a9,1,$a80,*-$800" LINE_ENDING);
    m_pAssembler = Assembler_CreateFromFile("." SLASH_STR "AssemblerTest", NULL);
    runAssemblerAndValidateLastLineIs("    :              3  usr $a9,1,$a80,*-$800" LINE_ENDING, 3);
    validateRW18ObjectFileContains(g_usrFilename, 0xa9, 1, 0xa80, "\x00\xff", 2);
}

TEST(AssemblerDirectives, USR_DirectiveFailsWhenLessThan4ArgumentsInOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800" LINE_ENDING
                                                   " hex 00,ff" LINE_ENDING
                                                   " usr $a9,1,$a80" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:3: error: '$a9,1,$a80' doesn't contain the 4 arguments required for USR directive." LINE_ENDING, 
                                   "    :              3  usr $a9,1,$a80" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, USR_DirectiveFailsWhenMoreThan4ArgumentsInOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800" LINE_ENDING
                                                   " hex 00,ff" LINE_ENDING
                                                   " usr $a9,1,$a80,*-$800,5" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:3: error: '$a9,1,$a80,*-$800,5' doesn't contain the 4 arguments required for USR directive." LINE_ENDING, 
                                   "    :              3  usr $a9,1,$a80,*-$800,5" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, USR_DirectiveFailsBinaryBufferWriteQueueingOperation)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800" LINE_ENDING
                                                   " hex 00,ff" LINE_ENDING
                                                   " usr $a9,1,$a80,*-$800" LINE_ENDING), NULL);
    MallocFailureInject_FailAllocation(4);
    __try_and_catch( Assembler_Run(m_pAssembler) );
    validateFailureOutput("filename:3: error: Failed to queue up USR save to 'filename'." LINE_ENDING, 
                          "    :              3  usr $a9,1,$a80,*-$800" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, DO_DirectiveWithOneExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1" LINE_ENDING
                                                   " hex 00" LINE_ENDING
                                                   " fin" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 00           2  hex 00" LINE_ENDING,
                                                   "    :              3  fin" LINE_ENDING, 3);
}

TEST(AssemblerDirectives, DO_DirectiveWithMaxShortExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do $ffff" LINE_ENDING
                                                   " hex 00" LINE_ENDING
                                                   " fin" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 00           2  hex 00" LINE_ENDING,
                                                   "    :              3  fin" LINE_ENDING, 3);
}

TEST(AssemblerDirectives, DO_DirectiveWithZeroExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 0" LINE_ENDING
                                                   " hex 00" LINE_ENDING
                                                   " fin" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              2  hex 00" LINE_ENDING,
                                                   "    :              3  fin" LINE_ENDING, 3);
}

TEST(AssemblerDirectives, DO_DirectivewithZeroExpressionButContinueToAssembleAfterFIN)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 0" LINE_ENDING
                                                   " hex 00" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " hex 01" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              3  fin" LINE_ENDING,
                                                   "8000: 01           4  hex 01" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, DO_DirectiveWithDivExpressionThatResultsInNonZero)
{
    m_pAssembler = Assembler_CreateFromString(dupe("two equ 2" LINE_ENDING
                                                   " do 2/two" LINE_ENDING
                                                   " hex 00" LINE_ENDING
                                                   " fin" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 00           3  hex 00" LINE_ENDING,
                                                   "    :              4  fin" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, DO_DirectiveWithDivExpressionThatResultsInZero)
{
    m_pAssembler = Assembler_CreateFromString(dupe("two equ 2" LINE_ENDING
                                                   " do 1/two" LINE_ENDING
                                                   " hex 00" LINE_ENDING
                                                   " fin" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              3  hex 00" LINE_ENDING,
                                                   "    :              4  fin" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, DO_DirectiveWith0Else)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 0" LINE_ENDING
                                                   " hex 00" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   " hex 01" LINE_ENDING
                                                   " fin" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 01           4  hex 01" LINE_ENDING,
                                                   "    :              5  fin" LINE_ENDING, 5);
}

TEST(AssemblerDirectives, DO_DirectiveWith1Else)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1" LINE_ENDING
                                                   " hex 00" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   " hex 01" LINE_ENDING
                                                   " fin" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              4  hex 01" LINE_ENDING,
                                                   "    :              5  fin" LINE_ENDING, 5);
}

TEST(AssemblerDirectives, DO_DirectiveWith2LevelNestingTest00)
{
    m_pAssembler = Assembler_CreateFromString(dupe("test1 equ 0" LINE_ENDING
                                                   "test2 equ 0" LINE_ENDING
                                                   " do test1" LINE_ENDING
                                                   " do test2" LINE_ENDING
                                                   "label equ 1" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   "label equ 2" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   " do test2" LINE_ENDING
                                                   "label equ 3" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   "label equ 4" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " db label" LINE_ENDING), NULL);
    runAssemblerAndValidateLastLineIs("8000: 04          16  db label" LINE_ENDING, 16);
}

TEST(AssemblerDirectives, DO_DirectiveWith2LevelNestingTest01)
{
    m_pAssembler = Assembler_CreateFromString(dupe("test1 equ 0" LINE_ENDING
                                                   "test2 equ 1" LINE_ENDING
                                                   " do test1" LINE_ENDING
                                                   " do test2" LINE_ENDING
                                                   "label equ 1" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   "label equ 2" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   " do test2" LINE_ENDING
                                                   "label equ 3" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   "label equ 4" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " db label" LINE_ENDING), NULL);
    runAssemblerAndValidateLastLineIs("8000: 03          16  db label" LINE_ENDING, 16);
}

TEST(AssemblerDirectives, DO_DirectiveWith2LevelNestingTest10)
{
    m_pAssembler = Assembler_CreateFromString(dupe("test1 equ 1" LINE_ENDING
                                                   "test2 equ 0" LINE_ENDING
                                                   " do test1" LINE_ENDING
                                                   " do test2" LINE_ENDING
                                                   "label equ 1" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   "label equ 2" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   " do test2" LINE_ENDING
                                                   "label equ 3" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   "label equ 4" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " db label" LINE_ENDING), NULL);
    runAssemblerAndValidateLastLineIs("8000: 02          16  db label" LINE_ENDING, 16);
}

TEST(AssemblerDirectives, DO_DirectiveWith2LevelNestingTest11)
{
    m_pAssembler = Assembler_CreateFromString(dupe("test1 equ 1" LINE_ENDING
                                                   "test2 equ 1" LINE_ENDING
                                                   " do test1" LINE_ENDING
                                                   " do test2" LINE_ENDING
                                                   "label equ 1" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   "label equ 2" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   " do test2" LINE_ENDING
                                                   "label equ 3" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   "label equ 4" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " db label" LINE_ENDING), NULL);
    runAssemblerAndValidateLastLineIs("8000: 01          16  db label" LINE_ENDING, 16);
}

TEST(AssemblerDirectives, DO_DirectiveWith8LevelsOfNesting)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1" LINE_ENDING
                                                   " do 1" LINE_ENDING
                                                   " do 1" LINE_ENDING
                                                   " do 1" LINE_ENDING
                                                   " do 1" LINE_ENDING
                                                   " do 1" LINE_ENDING
                                                   " do 1" LINE_ENDING
                                                   " do 1" LINE_ENDING
                                                   "label equ $ff" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " db label" LINE_ENDING), NULL);
    runAssemblerAndValidateLastLineIs("8000: FF          18  db label" LINE_ENDING, 18);
}

TEST(AssemblerDirectives, DO_DirectiveWithForwardReferenceInClause)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1" LINE_ENDING
                                                   " db label" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   "label equ $ff"), NULL);
    Assembler_Run(m_pAssembler);

    LineInfo* pSecondLine = m_pAssembler->linesHead.pNext->pNext;
    LONGS_EQUAL(1, pSecondLine->machineCodeSize);
    LONGS_EQUAL(0, memcmp(pSecondLine->pMachineCode, "\xff", 1));
}

TEST(AssemblerDirectives, DO_DirectiveWithNoExpressionIsInvalid)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: do directive requires operand." LINE_ENDING, 
                                    "    :              1  do" LINE_ENDING, 2);
}

TEST(AssemblerDirectives, DO_DirectiveWithForwardExpressionIsInvalid)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do ForwardLabel" LINE_ENDING
                                                   "ForwardLabel equ 1" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: do directive can't forward reference labels." LINE_ENDING, 
                                   "    :    =0001     2 ForwardLabel equ 1" LINE_ENDING, 3);
}

TEST(AssemblerDirectives, ELSE_DirectiveWithExpressionIsInvalid)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1" LINE_ENDING
                                                   " hex 00" LINE_ENDING
                                                   " else 0" LINE_ENDING
                                                   " hex 01" LINE_ENDING
                                                   " fin" LINE_ENDING), NULL);
    runAssemblerAndValidateWarning("filename:3: warning: else directive ignoring operand as comment." LINE_ENDING, 
                                   "    :              5  fin" LINE_ENDING, 6);
}

TEST(AssemblerDirectives, FIN_DirectiveWarningForExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1" LINE_ENDING
                                                   " hex 00" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   " hex 01" LINE_ENDING
                                                   " fin 1" LINE_ENDING), NULL);
    runAssemblerAndValidateWarning("filename:5: warning: fin directive ignoring operand as comment." LINE_ENDING, 
                                    "    :              5  fin 1" LINE_ENDING, 6);
}

TEST(AssemblerDirectives, DO_DirectiveWithNoMatchingFIN)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1" LINE_ENDING
                                                   " do 1" LINE_ENDING
                                                   " fin" LINE_ENDING), NULL);
    runAssemblerAndValidateWarning("filename:1: warning: DO/IF directive is missing matching FIN directive." LINE_ENDING, 
                                   "    :              3  fin" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, FIN_DirectiveWithNoMatchingDO)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " fin" LINE_ENDING), NULL);
    runAssemblerAndValidateWarning("filename:3: warning: fin directive without corresponding DO/IF directive." LINE_ENDING, 
                                   "    :              3  fin" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, ELSE_DirectiveWithNoMatchingDO)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1" LINE_ENDING
                                                   " fin" LINE_ENDING
                                                   " else" LINE_ENDING), NULL);
    runAssemblerAndValidateWarning("filename:3: warning: else directive without corresponding DO/IF directive." LINE_ENDING, 
                                   "    :              3  else" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, ELSE_DirectiveWhenELSEAlreadyUsed)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   " else" LINE_ENDING
                                                   " fin" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:3: error: Can't have multiple ELSE directives in a DO/IF clause." LINE_ENDING, 
                                   "    :              4  fin" LINE_ENDING, 5);
}

TEST(AssemblerDirectives, DO_DirectiveWithFailedAllocationForConditional)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" do 1" LINE_ENDING), NULL);
    MallocFailureInject_FailAllocation(2);
    __try_and_catch( Assembler_Run(m_pAssembler) );
    validateFailureOutput("filename:1: error: Failed to allocate space for DO conditional storage." LINE_ENDING, 
                          "    :              1  do 1" LINE_ENDING, 2);
}

TEST(AssemblerDirectives, LUP_DirectiveWith1Iteration)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lup 1" LINE_ENDING
                                                   " hex ff" LINE_ENDING
                                                   " --^" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: FF               2  hex ff" LINE_ENDING,
                                                   "    :              3  --^" LINE_ENDING, 3);
}

TEST(AssemblerDirectives, LUP_DirectiveWith2Iterations)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lup 2" LINE_ENDING
                                                   " hex ff" LINE_ENDING
                                                   " --^" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8001: FF               2  hex ff" LINE_ENDING,
                                                   "    :              3  --^" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, LUP_DirectiveWith2IterationsAndContinueAssemblyProcess)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lup 2" LINE_ENDING
                                                   " hex ff" LINE_ENDING
                                                   " --^" LINE_ENDING
                                                   " hex 00" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              3  --^" LINE_ENDING,
                                                   "8002: 00           4  hex 00" LINE_ENDING, 5);
}

TEST(AssemblerDirectives, LUP_DirectiveWith8000Iterations)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $0" LINE_ENDING
                                                   " lup $8000" LINE_ENDING
                                                   " hex ff" LINE_ENDING
                                                   " --^" LINE_ENDING), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("7FFF: FF               3  hex ff" LINE_ENDING,
                                                   "    :              4  --^" LINE_ENDING, 0x8000 + 3);
}

TEST(AssemblerDirectives, LUP_DirectiveWithInvalidIterationCountOf8001)
{
    m_pAssembler = Assembler_CreateFromString(" org $0" LINE_ENDING
                                              " lup $8001" LINE_ENDING
                                              " hex ff" LINE_ENDING, NULL);
    runAssemblerAndValidateWarning("filename:2: warning: LUP directive count of 32769 doesn't fall in valid range of 1 to 32768." LINE_ENDING, 
                                   "0000: FF           3  hex ff" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, LUP_DirectiveWithInvalidIterationCountOf0)
{
    m_pAssembler = Assembler_CreateFromString(" org $0" LINE_ENDING
                                              " lup 0" LINE_ENDING
                                              " hex ff" LINE_ENDING, NULL);
    runAssemblerAndValidateWarning("filename:2: warning: LUP directive count of 0 doesn't fall in valid range of 1 to 32768." LINE_ENDING, 
                                   "0000: FF           3  hex ff" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, LUP_DirectiveFailByTerminatingTwice)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lup 1" LINE_ENDING
                                                   " hex ff" LINE_ENDING
                                                   " --^" LINE_ENDING
                                                   " --^" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:4: error: --^ directive without corresponding LUP directive." LINE_ENDING, 
                                   "    :              4  --^" LINE_ENDING, 5);
}

TEST(AssemblerDirectives, LUP_DirectiveFailByMissingTerminatingDirective)
{
    m_pAssembler = Assembler_CreateFromString(" lup 1" LINE_ENDING
                                              " hex ff" LINE_ENDING, NULL);
    runAssemblerAndValidateFailure("filename:1: error: LUP directive is missing matching --^ directive." LINE_ENDING, 
                                   "8000: FF           2  hex ff" LINE_ENDING, 3);
}

TEST(AssemblerDirectives, LUP_DirectiveFailWithNoExpression)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lup" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: lup directive requires operand." LINE_ENDING, 
                                   "    :              1  lup" LINE_ENDING, 2);
}

TEST(AssemblerDirectives, LUPend_DirectiveWarningWithExpression)
{
    m_pAssembler = Assembler_CreateFromString(" lup 1" LINE_ENDING
                                              " hex ff" LINE_ENDING
                                              " --^ 1" LINE_ENDING, NULL);
    runAssemblerAndValidateWarning("filename:3: warning: --^ directive ignoring operand as comment." LINE_ENDING, 
                                   "    :              3  --^ 1" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, LUPend_DirectiveWarningWithComment)
{
    m_pAssembler = Assembler_CreateFromString(" lup 1" LINE_ENDING
                                              " hex ff" LINE_ENDING
                                              " --^ ; Comment" LINE_ENDING, NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: FF               2  hex ff" LINE_ENDING,
                                                   "    :              3  --^ ; Comment" LINE_ENDING, 3);
}

TEST(AssemblerDirectives, LUP_DirectiveFailExpressionThatContainsForwardReferences)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" lup forward" LINE_ENDING
                                                   " hex ff" LINE_ENDING
                                                   " --^" LINE_ENDING
                                                   "forward equ 5" LINE_ENDING), NULL);
    runAssemblerAndValidateFailure("filename:1: error: lup directive can't forward reference labels." LINE_ENDING, 
                                   "    :    =0005     4 forward equ 5" LINE_ENDING, 5);
}

TEST(AssemblerDirectives, LUP_FailTextFileAllocation)
{
    m_pAssembler = Assembler_CreateFromString(" lup 1" LINE_ENDING
                                              " hex ff" LINE_ENDING
                                              " --^" LINE_ENDING, NULL);
    MallocFailureInject_FailAllocation(2);
    runAssemblerAndValidateFailure("filename:1: error: Failed to allocate memory for LUP directive." LINE_ENDING, 
                                   "    :              3  --^" LINE_ENDING, 4);
}

TEST(AssemblerDirectives, LUP_FailLupSourceAllocation)
{
    m_pAssembler = Assembler_CreateFromString(" lup 1" LINE_ENDING
                                              " hex ff" LINE_ENDING
                                              " --^" LINE_ENDING, NULL);
    MallocFailureInject_FailAllocation(3);
    runAssemblerAndValidateFailure("filename:1: error: Failed to allocate memory for LUP directive." LINE_ENDING, 
                                   "    :              3  --^" LINE_ENDING, 3);
}
