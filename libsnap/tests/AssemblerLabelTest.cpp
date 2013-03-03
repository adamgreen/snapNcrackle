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
/* Assembler module tests related to labels.  This include symbol creation for basic line labels and EQU statements.  It
   also includes the testing of forward and backward label references in assembly language code.
*/
#include "AssemblerBaseTest.h"


TEST_GROUP_BASE(AssemblerLabel, AssemblerBase)
{
};


TEST(AssemblerLabel, SpecifySameLabelTwice)
{
    m_pAssembler = Assembler_CreateFromString(dupe("entry lda #$60\n"
                                                   "entry lda #$61\n"), NULL);
    runAssemblerAndValidateFailure("filename:2: error: 'entry' symbol has already been defined.\n",
                                   "8002: A9 61        2 entry lda #$61\n", 3);
}

TEST(AssemblerLabel, LocalLabelDefineBeforeGlobalLabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe(":local_label\n"), NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: ':local_label' local label isn't allowed before first global label.\n", printfSpy_GetPreviousOutput());
}

TEST(AssemblerLabel, LocalLabelReferenceBeforeGlobalLabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta :local_label\n"), NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: ':local_label' local label isn't allowed before first global label.\n", printfSpy_GetPreviousOutput());
}

TEST(AssemblerLabel, LabelStartsWithInvalidCharacter)
{
    m_pAssembler = Assembler_CreateFromString(dupe("9Label sta $23\n"), NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: '9Label' label starts with invalid character.\n", printfSpy_GetPreviousOutput());
}

TEST(AssemblerLabel, LabelContainsInvalidCharacter)
{
    m_pAssembler = Assembler_CreateFromString(dupe("Label. sta $23\n"), NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: 'Label.' label contains invalid character, '.'.\n", printfSpy_GetPreviousOutput());
}

TEST(AssemblerLabel, ForwardReferenceLabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " sta label\n"
                                                   "label sta $2b\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("0800: 8D 03 08     2  sta label\n",
                                                   "0803: 85 2B        3 label sta $2b\n", 3);
}

TEST(AssemblerLabel, FailToDefineLabelTwiceAfterForwardReferenced)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " sta label\n"
                                                   "label sta $2b\n"
                                                   "label sta $2c\n"), NULL);
    runAssemblerAndValidateFailure("filename:4: error: 'label' symbol has already been defined.\n",
                                   "0805: 85 2C        4 label sta $2c\n", 5);
}

TEST(AssemblerLabel, LocalLabelPlusOffsetBackwardReference)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $0000\n"
                                                   "func1 sta $20\n"
                                                   ":local sta $20\n"
                                                   "func2 sta $21\n"
                                                   ":local sta $22\n"
                                                   " sta :local+1\n"), NULL);
    runAssemblerAndValidateLastLineIs("0008: 85 07        6  sta :local+1\n", 6);
}

TEST(AssemblerLabel, LocalLabelPlusOffsetForwardReference)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   "func1 sta $20\n"
                                                   ":local sta $20\n"
                                                   "func2 sta $21\n"
                                                   " sta :local+1\n"
                                                   ":local sta $22\n"), NULL);
    
    runAssemblerAndValidateLastTwoLinesOfOutputAre("0806: 8D 0A 08     5  sta :local+1\n",
                                                   "0809: 85 22        6 :local sta $22\n", 6);
}

TEST(AssemblerLabel, LocalLabelForwardReferenceFromLineWithGlobalLabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe("func1 sta :local\n"
                                                   ":local sta $20\n"), NULL);
    
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 8D 03 80     1 func1 sta :local\n",
                                                   "8003: 85 20        2 :local sta $20\n", 2);
}

TEST(AssemblerLabel, GlobalLabelPlusOffsetForwardReference)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " sta globalLabel+1\n"
                                                   "globalLabel sta $22\n"), NULL);
    
    runAssemblerAndValidateLastTwoLinesOfOutputAre("0800: 8D 04 08     2  sta globalLabel+1\n",
                                                   "0803: 85 22        3 globalLabel sta $22\n", 3);
}

TEST(AssemblerLabel, CascadedForwardReferenceOfEQUToLabel)
{
    LineInfo* pSecondLine;
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " sta 1+equLabel\n"
                                                   "equLabel equ lineLabel\n"
                                                   "lineLabel sta $22\n"), NULL);
    Assembler_Run(m_pAssembler);
    pSecondLine = m_pAssembler->linesHead.pNext->pNext;
    LONGS_EQUAL(3, pSecondLine->machineCodeSize);
    CHECK(0 == memcmp(pSecondLine->pMachineCode, "\x8d\x04\x08", 3));
}

TEST(AssemblerLabel, MultipleForwardReferencesToSameLabel)
{
    LineInfo* pSecondLine;
    LineInfo* pThirdLine;
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " sta 1+label\n"
                                                   " sta label+1\n"
                                                   "label sta $22\n"), NULL);
    Assembler_Run(m_pAssembler);
    pSecondLine = m_pAssembler->linesHead.pNext->pNext;
    LONGS_EQUAL(3, pSecondLine->machineCodeSize);
    CHECK(0 == memcmp(pSecondLine->pMachineCode, "\x8d\x07\x08", 3));
    pThirdLine = pSecondLine->pNext;
    LONGS_EQUAL(3, pThirdLine->machineCodeSize);
    CHECK(0 == memcmp(pThirdLine->pMachineCode, "\x8d\x07\x08", 3));
}

TEST(AssemblerLabel, MultipleBackReferencesToSameVariable)
{
    LineInfo* pSecondLine;
    LineInfo* pFourthLine;
    m_pAssembler = Assembler_CreateFromString(dupe("]variable ds 1\n"
                                                   " sta ]variable\n"
                                                   "]variable ds 1\n"
                                                   " sta ]variable\n"), NULL);
    Assembler_Run(m_pAssembler);
    pSecondLine = m_pAssembler->linesHead.pNext->pNext;
    LONGS_EQUAL(3, pSecondLine->machineCodeSize);
    CHECK(0 == memcmp(pSecondLine->pMachineCode, "\x8d\x00\x80", 3));
    pFourthLine = pSecondLine->pNext->pNext;
    LONGS_EQUAL(3, pFourthLine->machineCodeSize);
    CHECK(0 == memcmp(pFourthLine->pMachineCode, "\x8d\x04\x80", 3));
}

TEST(AssemblerLabel, ForwardReferencesToVariableWithMultipleDefinitionsUsesFirstDefinition)
{
    LineInfo* pFirstLine;
    m_pAssembler = Assembler_CreateFromString(dupe(" sta ]variable\n"
                                                   "]variable ds 1\n"
                                                   "]varaible ds 1\n"), NULL);
    Assembler_Run(m_pAssembler);
    pFirstLine = m_pAssembler->linesHead.pNext;
    LONGS_EQUAL(3, pFirstLine->machineCodeSize);
    CHECK(0 == memcmp(pFirstLine->pMachineCode, "\x8d\x03\x80", 3));
}

TEST(AssemblerLabel, Variable0DefaultTo0Value)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta ]0\n"), NULL);
    runAssemblerAndValidateLastLineIs("8000: 85 00        1  sta ]0\n", 1);
}

TEST(AssemblerLabel, Variable9DefaultTo0Value)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta ]9\n"), NULL);
    runAssemblerAndValidateLastLineIs("8000: 85 00        1  sta ]9\n", 1);
}

TEST(AssemblerLabel, FailZeroPageForwardReference)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $0000\n"
                                                   " sta globalLabel\n"
                                                   "globalLabel sta $22\n"), NULL);
    
    runAssemblerAndValidateFailure("filename:2: error: Couldn't properly infer size of a forward reference in 'globalLabel' operand.\n",
                                   "0003: 85 22        3 globalLabel sta $22\n", 4);
}

TEST(AssemblerLabel, ReferenceNonExistantLabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta badLabel\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: The 'badLabel' label is undefined.\n",
                                   "8000: 8D 00 00     1  sta badLabel\n", 2);
}

TEST(AssemblerLabel, EQUMissingLineLabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" EQU $23\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: EQU directive requires a line label.\n",
                                   "    :              1  EQU $23\n", 2);
}

TEST(AssemblerLabel, EQULabelStartsWithInvalidCharacter)
{
    m_pAssembler = Assembler_CreateFromString(dupe("9Label EQU $23\n"), NULL);
    CHECK(m_pAssembler != NULL);

    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: '9Label' label starts with invalid character.\n", printfSpy_GetPreviousOutput());
}

TEST(AssemblerLabel, EQULabelContainsInvalidCharacter)
{
    m_pAssembler = Assembler_CreateFromString(dupe("Label. EQU $23\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: 'Label.' label contains invalid character, '.'.\n", 
                                   "    :              1 Label. EQU $23\n");
}

TEST(AssemblerLabel, EQULabelIsLocal)
{
    m_pAssembler = Assembler_CreateFromString(dupe("Global\n"
                                                   ":Label EQU $23\n"), NULL);

    runAssemblerAndValidateFailure("filename:2: error: ':Label' can't be a local label when used with EQU.\n", 
                                   "    :              2 :Label EQU $23\n", 3);
}

TEST(AssemblerLabel, ForwardReferenceEQULabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta label\n"
                                                   "label equ $ffff\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 8D FF FF     1  sta label\n",
                                                   "    :    =FFFF     2 label equ $ffff\n");
}

TEST(AssemblerLabel, FailToDefineEquLabelTwiceAfterForwardReferenced)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" sta label\n"
                                                   "label equ $ff2b\n"
                                                   "label equ $ff2c\n"), NULL);
    runAssemblerAndValidateFailure("filename:3: error: 'label' symbol has already been defined.\n",
                                   "    :              3 label equ $ff2c\n", 4);
}

TEST(AssemblerLabel, EqualSignDirective)
{
    m_pAssembler = Assembler_CreateFromString(dupe("org = $800\n"), NULL);
    runAssemblerAndValidateOutputIs("    :    =0800     1 org = $800\n");
}

TEST(AssemblerLabel, EqualSignMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe("Label =\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: = directive requires operand.\n",
                                   "    :              1 Label =\n", 2);
}

TEST(AssemblerLabel, EQUDirective)
{
    m_pAssembler = Assembler_CreateFromString(dupe("org EQU $800\n"), NULL);
    runAssemblerAndValidateOutputIs("    :    =0800     1 org EQU $800\n");
}

TEST(AssemblerLabel, EQUDirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString(dupe("Label EQU\n"), NULL);
    runAssemblerAndValidateFailure("filename:1: error: EQU directive requires operand.\n",
                                   "    :              1 Label EQU\n", 2);
}

TEST(AssemblerLabel, MultipleDefinedSymbolFailure)
{
    m_pAssembler = Assembler_CreateFromString(dupe("org = $800\n"
                                                   "org EQU $900\n"), NULL);
    runAssemblerAndValidateFailure("filename:2: error: 'org' symbol has already been defined.\n", 
                                   "    :              2 org EQU $900\n", 3);
}

TEST(AssemblerLabel, FailAllocationDuringSymbolCreation)
{
    m_pAssembler = Assembler_CreateFromString(dupe("org = $800\n"), NULL);
    MallocFailureInject_FailAllocation(2);
    runAssemblerAndValidateFailure("filename:1: error: Failed to allocate space for 'org' symbol.\n", 
                                   "    :              1 org = $800\n");
}

TEST(AssemblerLabel, VerifyObjectFileWithForwardReferenceLabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   " sta label\n"
                                                   "label sta $2b\n"
                                                   " sav AssemblerTest.sav\n"), NULL);
    Assembler_Run(m_pAssembler);
    validateObjectFileContains(0x800, "\x8d\x03\x08\x85\x2b", 5);
}

TEST(AssemblerLabel, STAAbsoluteViaLabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $800\n"
                                                   "entry lda #$60\n"
                                                   " sta entry\n"), NULL);
    runAssemblerAndValidateLastLineIs("0802: 8D 00 08     3  sta entry\n", 3);
}

TEST(AssemblerLabel, STAZeroPageAbsoluteViaLabel)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $0000\n"
                                                   "entry lda #$60\n"
                                                   " sta entry\n"), NULL);
    runAssemblerAndValidateLastLineIs("0002: 85 00        3  sta entry\n", 3);
}
