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
/* Assembler module tests related to labels.  This include symbol creation for basic line labels and EQU statements.  It
   also includes the testing of forward and backward label references in assembly language code.
*/
#include "AssemblerBaseTest.h"


TEST_GROUP_BASE(AssemblerLabel, AssemblerBase)
{
};


TEST(AssemblerLabel, SpecifySameLabelTwice)
{
    m_pAssembler = Assembler_CreateFromString("entry lda #$60" LINE_ENDING
                                              "entry lda #$61" LINE_ENDING, NULL);
    runAssemblerAndValidateFailure("filename:2: error: 'entry' symbol has already been defined." LINE_ENDING,
                                   "8002: A9 61        2 entry lda #$61" LINE_ENDING, 3);
}

TEST(AssemblerLabel, LocalLabelDefineBeforeGlobalLabel)
{
    m_pAssembler = Assembler_CreateFromString(":local_label" LINE_ENDING, NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: ':local_label' local label isn't allowed before first global label." LINE_ENDING, printfSpy_GetPreviousOutput());
}

TEST(AssemblerLabel, LocalLabelReferenceBeforeGlobalLabel)
{
    m_pAssembler = Assembler_CreateFromString(" sta :local_label" LINE_ENDING, NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: ':local_label' local label isn't allowed before first global label." LINE_ENDING, printfSpy_GetPreviousOutput());
}

TEST(AssemblerLabel, LabelStartsWithInvalidCharacter)
{
    m_pAssembler = Assembler_CreateFromString("9Label sta $23" LINE_ENDING, NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: '9Label' label starts with invalid character." LINE_ENDING, printfSpy_GetPreviousOutput());
}

TEST(AssemblerLabel, LabelContainsInvalidCharacter)
{
    m_pAssembler = Assembler_CreateFromString("Label. sta $23" LINE_ENDING, NULL);
    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: 'Label.' label contains invalid character, '.'." LINE_ENDING, printfSpy_GetPreviousOutput());
}

TEST(AssemblerLabel, ForwardReferenceLabel)
{
    m_pAssembler = Assembler_CreateFromString(" org $800" LINE_ENDING
                                              " sta label" LINE_ENDING
                                              "label sta $2b" LINE_ENDING, NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("0800: 8D 03 08     2  sta label" LINE_ENDING,
                                                   "0803: 85 2B        3 label sta $2b" LINE_ENDING, 3);
}

TEST(AssemblerLabel, FailToDefineLabelTwiceAfterForwardReferenced)
{
    m_pAssembler = Assembler_CreateFromString(" org $800" LINE_ENDING
                                              " sta label" LINE_ENDING
                                              "label sta $2b" LINE_ENDING
                                              "label sta $2c" LINE_ENDING, NULL);
    runAssemblerAndValidateFailure("filename:4: error: 'label' symbol has already been defined." LINE_ENDING,
                                   "0805: 85 2C        4 label sta $2c" LINE_ENDING, 5);
}

TEST(AssemblerLabel, LocalLabelPlusOffsetBackwardReference)
{
    m_pAssembler = Assembler_CreateFromString(" org $0000" LINE_ENDING
                                              "func1 sta $20" LINE_ENDING
                                              ":local sta $20" LINE_ENDING
                                              "func2 sta $21" LINE_ENDING
                                              ":local sta $22" LINE_ENDING
                                              " sta :local+1" LINE_ENDING, NULL);
    runAssemblerAndValidateLastLineIs("0008: 85 07        6  sta :local+1" LINE_ENDING, 6);
}

TEST(AssemblerLabel, LocalLabelPlusOffsetForwardReference)
{
    m_pAssembler = Assembler_CreateFromString(" org $800" LINE_ENDING
                                              "func1 sta $20" LINE_ENDING
                                              ":local sta $20" LINE_ENDING
                                              "func2 sta $21" LINE_ENDING
                                              " sta :local+1" LINE_ENDING
                                              ":local sta $22" LINE_ENDING, NULL);
    
    runAssemblerAndValidateLastTwoLinesOfOutputAre("0806: 8D 0A 08     5  sta :local+1" LINE_ENDING,
                                                   "0809: 85 22        6 :local sta $22" LINE_ENDING, 6);
}

TEST(AssemblerLabel, LocalLabelForwardReferenceFromLineWithGlobalLabel)
{
    m_pAssembler = Assembler_CreateFromString("func1 sta :local" LINE_ENDING
                                              ":local sta $20" LINE_ENDING, NULL);
    
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 8D 03 80     1 func1 sta :local" LINE_ENDING,
                                                   "8003: 85 20        2 :local sta $20" LINE_ENDING, 2);
}

TEST(AssemblerLabel, LocalLabelReferenceToSelf)
{
    m_pAssembler = Assembler_CreateFromString("global" LINE_ENDING
                                              ":local jmp :local" LINE_ENDING, NULL);
    
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              1 global" LINE_ENDING,
                                                   "8000: 4C 00 80     2 :local jmp :local" LINE_ENDING, 2);
}

TEST(AssemblerLabel, GlobalLabelPlusOffsetForwardReference)
{
    m_pAssembler = Assembler_CreateFromString(" org $800" LINE_ENDING
                                              " sta globalLabel+1" LINE_ENDING
                                              "globalLabel sta $22" LINE_ENDING, NULL);
    
    runAssemblerAndValidateLastTwoLinesOfOutputAre("0800: 8D 04 08     2  sta globalLabel+1" LINE_ENDING,
                                                   "0803: 85 22        3 globalLabel sta $22" LINE_ENDING, 3);
}

TEST(AssemblerLabel, GlobalLabelReferenceToSelf)
{
    m_pAssembler = Assembler_CreateFromString("global jmp global" LINE_ENDING, NULL);
    
    runAssemblerAndValidateOutputIs("8000: 4C 00 80     1 global jmp global" LINE_ENDING);
}

TEST(AssemblerLabel, CascadedForwardReferenceOfEQUToLabel)
{
    LineInfo* pSecondLine;
    m_pAssembler = Assembler_CreateFromString(" org $800" LINE_ENDING
                                              " sta 1+equLabel" LINE_ENDING
                                              "equLabel equ lineLabel" LINE_ENDING
                                              "lineLabel sta $22" LINE_ENDING, NULL);
    Assembler_Run(m_pAssembler);
    pSecondLine = m_pAssembler->linesHead.pNext->pNext;
    LONGS_EQUAL(3, pSecondLine->machineCodeSize);
    CHECK(0 == memcmp(pSecondLine->pMachineCode, "\x8d\x04\x08", 3));
}

TEST(AssemblerLabel, MultipleForwardReferencesToSameLabel)
{
    LineInfo* pSecondLine;
    LineInfo* pThirdLine;
    m_pAssembler = Assembler_CreateFromString(" org $800" LINE_ENDING
                                              " sta 1+label" LINE_ENDING
                                              " sta label+1" LINE_ENDING
                                              "label sta $22" LINE_ENDING, NULL);
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
    m_pAssembler = Assembler_CreateFromString("]variable ds 1" LINE_ENDING
                                              " sta ]variable" LINE_ENDING
                                              "]variable ds 1" LINE_ENDING
                                              " sta ]variable" LINE_ENDING, NULL);
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
    m_pAssembler = Assembler_CreateFromString(" sta ]variable" LINE_ENDING
                                              "]variable ds 1" LINE_ENDING
                                              "]varaible ds 1" LINE_ENDING, NULL);
    Assembler_Run(m_pAssembler);
    pFirstLine = m_pAssembler->linesHead.pNext;
    LONGS_EQUAL(3, pFirstLine->machineCodeSize);
    CHECK(0 == memcmp(pFirstLine->pMachineCode, "\x8d\x03\x80", 3));
}

TEST(AssemblerLabel, Variable0DefaultTo0Value)
{
    m_pAssembler = Assembler_CreateFromString(" sta ]0" LINE_ENDING, NULL);
    runAssemblerAndValidateLastLineIs("8000: 85 00        1  sta ]0" LINE_ENDING, 1);
}

TEST(AssemblerLabel, Variable9DefaultTo0Value)
{
    m_pAssembler = Assembler_CreateFromString(" sta ]9" LINE_ENDING, NULL);
    runAssemblerAndValidateLastLineIs("8000: 85 00        1  sta ]9" LINE_ENDING, 1);
}

TEST(AssemblerLabel, FailZeroPageForwardReference)
{
    m_pAssembler = Assembler_CreateFromString(" org $0000" LINE_ENDING
                                              " sta globalLabel" LINE_ENDING
                                              "globalLabel sta $22" LINE_ENDING, NULL);
    
    runAssemblerAndValidateFailure("filename:2: error: Couldn't properly infer size of a forward reference in 'globalLabel' operand." LINE_ENDING,
                                   "0003: 85 22        3 globalLabel sta $22" LINE_ENDING, 4);
}

TEST(AssemblerLabel, ReferenceNonExistantLabel)
{
    m_pAssembler = Assembler_CreateFromString(" sta badLabel" LINE_ENDING, NULL);
    runAssemblerAndValidateFailure("filename:1: error: The 'badLabel' label is undefined." LINE_ENDING,
                                   "8000: 8D 00 00     1  sta badLabel" LINE_ENDING, 2);
}

TEST(AssemblerLabel, EQUMissingLineLabel)
{
    m_pAssembler = Assembler_CreateFromString(" EQU $23" LINE_ENDING, NULL);
    runAssemblerAndValidateFailure("filename:1: error: EQU directive requires a line label." LINE_ENDING,
                                   "    :              1  EQU $23" LINE_ENDING, 2);
}

TEST(AssemblerLabel, EQULabelStartsWithInvalidCharacter)
{
    m_pAssembler = Assembler_CreateFromString("9Label EQU $23" LINE_ENDING, NULL);
    CHECK(m_pAssembler != NULL);

    Assembler_Run(m_pAssembler);
    LONGS_EQUAL(2, printfSpy_GetCallCount());
    LONGS_EQUAL(1, Assembler_GetErrorCount(m_pAssembler));
    STRCMP_EQUAL("filename:1: error: '9Label' label starts with invalid character." LINE_ENDING, printfSpy_GetPreviousOutput());
}

TEST(AssemblerLabel, EQULabelContainsInvalidCharacter)
{
    m_pAssembler = Assembler_CreateFromString("Label. EQU $23" LINE_ENDING, NULL);
    runAssemblerAndValidateFailure("filename:1: error: 'Label.' label contains invalid character, '.'." LINE_ENDING, 
                                   "    :    =0023     1 Label. EQU $23" LINE_ENDING);
}

TEST(AssemblerLabel, EQULabelIsLocal)
{
    m_pAssembler = Assembler_CreateFromString("Global" LINE_ENDING
                                                   ":Label EQU $23" LINE_ENDING, NULL);

    runAssemblerAndValidateFailure("filename:2: error: ':Label' can't be a local label when used with EQU." LINE_ENDING, 
                                   "    :              2 :Label EQU $23" LINE_ENDING, 3);
}

TEST(AssemblerLabel, EQULabelIsVariable)
{
    m_pAssembler = Assembler_CreateFromString("]byte = 0" LINE_ENDING, NULL);

    runAssemblerAndValidateOutputIs("    :    =0000     1 ]byte = 0" LINE_ENDING);
}

TEST(AssemblerLabel, EQUofSelfReferencingVariable)
{
    m_pAssembler = Assembler_CreateFromString("]byte = 0" LINE_ENDING
                                              "]byte = ]byte+1" LINE_ENDING, NULL);

    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :    =0000     1 ]byte = 0" LINE_ENDING,
                                                   "    :    =0001     2 ]byte = ]byte+1" LINE_ENDING);
}

TEST(AssemblerLabel, ForwardReferenceEQULabel)
{
    m_pAssembler = Assembler_CreateFromString(" sta label" LINE_ENDING
                                                   "label equ $ffff" LINE_ENDING, NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("8000: 8D FF FF     1  sta label" LINE_ENDING,
                                                   "    :    =FFFF     2 label equ $ffff" LINE_ENDING);
}

TEST(AssemblerLabel, FailToDefineEquLabelTwiceAfterForwardReferenced)
{
    m_pAssembler = Assembler_CreateFromString(" sta label" LINE_ENDING
                                              "label equ $ff2b" LINE_ENDING
                                              "label equ $ff2c" LINE_ENDING, NULL);
    runAssemblerAndValidateFailure("filename:3: error: 'label' symbol has already been defined." LINE_ENDING,
                                   "    :    =FF2C     3 label equ $ff2c" LINE_ENDING, 4);
}

TEST(AssemblerLabel, EqualSignDirective)
{
    m_pAssembler = Assembler_CreateFromString("org = $800" LINE_ENDING, NULL);
    runAssemblerAndValidateOutputIs("    :    =0800     1 org = $800" LINE_ENDING);
}

TEST(AssemblerLabel, EqualSignMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString("Label =" LINE_ENDING, NULL);
    runAssemblerAndValidateFailure("filename:1: error: = directive requires operand." LINE_ENDING,
                                   "    :              1 Label =" LINE_ENDING, 2);
}

TEST(AssemblerLabel, EQUDirective)
{
    m_pAssembler = Assembler_CreateFromString("org EQU $800" LINE_ENDING, NULL);
    runAssemblerAndValidateOutputIs("    :    =0800     1 org EQU $800" LINE_ENDING);
}

TEST(AssemblerLabel, EQUDirectiveMissingOperand)
{
    m_pAssembler = Assembler_CreateFromString("Label EQU" LINE_ENDING, NULL);
    runAssemblerAndValidateFailure("filename:1: error: EQU directive requires operand." LINE_ENDING,
                                   "    :              1 Label EQU" LINE_ENDING, 2);
}

TEST(AssemblerLabel, MultipleDefinedSymbolFailure)
{
    m_pAssembler = Assembler_CreateFromString("org = $800" LINE_ENDING
                                              "org EQU $900" LINE_ENDING, NULL);
    runAssemblerAndValidateFailure("filename:2: error: 'org' symbol has already been defined." LINE_ENDING, 
                                   "    :    =0900     2 org EQU $900" LINE_ENDING, 3);
}

TEST(AssemblerLabel, FailAllocationDuringSymbolCreation)
{
    m_pAssembler = Assembler_CreateFromString("org = $800" LINE_ENDING, NULL);
    MallocFailureInject_FailAllocation(2);
    runAssemblerAndValidateFailure("filename:1: error: Failed to allocate space for 'org' symbol." LINE_ENDING, 
                                   "    :    =0800     1 org = $800" LINE_ENDING);
}

TEST(AssemblerLabel, VerifyObjectFileWithForwardReferenceLabel)
{
    m_pAssembler = Assembler_CreateFromString(" org $800" LINE_ENDING
                                              " sta label" LINE_ENDING
                                              "label sta $2b" LINE_ENDING
                                              " sav AssemblerTest.sav" LINE_ENDING, NULL);
    Assembler_Run(m_pAssembler);
    validateObjectFileContains(0x800, "\x8d\x03\x08\x85\x2b", 5);
}

TEST(AssemblerLabel, STAAbsoluteViaLabel)
{
    m_pAssembler = Assembler_CreateFromString(" org $800" LINE_ENDING
                                              "entry lda #$60" LINE_ENDING
                                              " sta entry" LINE_ENDING, NULL);
    runAssemblerAndValidateLastLineIs("0802: 8D 00 08     3  sta entry" LINE_ENDING, 3);
}

TEST(AssemblerLabel, STAZeroPageAbsoluteViaLabel)
{
    m_pAssembler = Assembler_CreateFromString(" org $0000" LINE_ENDING
                                              "entry lda #$60" LINE_ENDING
                                              " sta entry" LINE_ENDING, NULL);
    runAssemblerAndValidateLastLineIs("0002: 85 00        3  sta entry" LINE_ENDING, 3);
}

TEST(AssemblerLabel, BackReferenceLocalLabelOnLineWithDo0)
{
    m_pAssembler = Assembler_CreateFromString("global" LINE_ENDING
                                              ":local do 0" LINE_ENDING
                                              " fin" LINE_ENDING
                                              " jmp :local" LINE_ENDING, NULL);
    
    runAssemblerAndValidateLastTwoLinesOfOutputAre("    :              3  fin" LINE_ENDING,
                                                   "8000: 4C 00 80     4  jmp :local" LINE_ENDING, 4);
}

TEST(AssemblerLabel, ForwardReferenceLocalLabelOnLineWithDo0)
{
    m_pAssembler = Assembler_CreateFromString("global beq :local" LINE_ENDING
                                              " hex ff" LINE_ENDING
                                              ":local do 0" LINE_ENDING
                                              " fin" LINE_ENDING
                                              " sav AssemblerTest.sav" LINE_ENDING, NULL);
    Assembler_Run(m_pAssembler);
    validateObjectFileContains(0x8000, "\xf0\x01\xff", 3);
}
