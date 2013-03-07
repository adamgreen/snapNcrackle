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
/* Tests for the 6502 and 65c02 instructions. */
#include "AssemblerBaseTest.h"


TEST_GROUP_BASE(AssemblerInstructions, AssemblerBase)
{
    int             m_isInvalidMode;
    int             m_isZeroPageTreatedAsAbsolute;
    int             m_isInvalidInstruction;
    int             m_switchTo65c02;
    unsigned char   m_expectedOpcode;

    void setup()
    {
        AssemblerBase::setup();
        m_isInvalidInstruction = FALSE;
        m_switchTo65c02 = FALSE;
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
            sprintf(expectedListOutput, "    :              1 %s", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
            return;
        }
        else
        {
            sprintf(expectedListOutput, "8000: %02X 7F        1 %s", opcode, testString);
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
            sprintf(expectedListOutput, "    :              1 %s", testString);
            validateFailureOutput(expectedErrorOutput, expectedListOutput);
        }
        else if (!m_isInvalidMode)
        {
            sprintf(expectedListOutput, 
                    "8000: %02X %s     1 %s", 
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
            sprintf(expectedListOutput, "    :              1 %s", testString);
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
        sprintf(expectedListOutput, "8000: %02X FE        1 %s", m_expectedOpcode, testString);
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
};


TEST(AssemblerInstructions, BEQ_ZeroPageMaxNegativeTarget)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $0090\n"
                                                   " beq *-126\n"), NULL);
    runAssemblerAndValidateLastLineIs("0090: F0 80        2  beq *-126\n", 2);
}

TEST(AssemblerInstructions, BEQ_ZeroPageMaxPositiveTarget)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $0000\n"
                                                   " beq *+129\n"), NULL);
    runAssemblerAndValidateLastLineIs("0000: F0 7F        2  beq *+129\n", 2);
}

TEST(AssemblerInstructions, BEQ_ZeroPageInvalidNegativeTarget)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $0090\n"
                                                   " beq *-127\n"), NULL);
    runAssemblerAndValidateFailure("filename:2: error: Relative offset of '*-127' exceeds the allowed -128 to 127 range.\n",
                                   "    :              2  beq *-127\n", 3);
}

TEST(AssemblerInstructions, BEQ_ZeroPageInvalidPositiveTarget)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $0000\n"
                                                   " beq *+130\n"), NULL);
    runAssemblerAndValidateFailure("filename:2: error: Relative offset of '*+130' exceeds the allowed -128 to 127 range.\n",
                                   "    :              2  beq *+130\n", 3);
}

TEST(AssemblerInstructions, BEQ_AbsoluteTarget)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $0800\n"
                                                   " beq *+2\n"), NULL);
    runAssemblerAndValidateLastLineIs("0800: F0 00        2  beq *+2\n", 2);
}

TEST(AssemblerInstructions, BEQ_ForwardLabelReference)
{
    m_pAssembler = Assembler_CreateFromString(dupe(" org $0800\n"
                                                   " beq label\n"
                                                   "label\n"), NULL);
    runAssemblerAndValidateLastTwoLinesOfOutputAre("0800: F0 00        2  beq label\n",
                                                   "    :              3 label\n", 3);
}



/* The comma separated list that is specified for each instruction is taken from the 65c02 data sheet and represents the
   addressing modes supported by that instruction and what opcode is used for a particular addressing mode.  The values
   are specified as two hexadecimal digits with some special values supported.  The special values include:
     XX indicates that this instruction doesn't support this corresponding addressing mode on the 6502 or 65c02.
     ^ prefix on a hex value indicates that this zero page based addressing mode isn't supported but the corresponding
       absolute mode with the specified hex opcode will be used by the assembler instead when code using this mode is
       encountered.
     * prefix indicates that this addressing mode is only supported on the 65c02 and not on the original 6502.
*/
TEST(AssemblerInstructions, ADC_TableDrivenTest)
{
    test6502_65c02Instruction("adc", "69,6D,65,XX,61,71,75,^79,7D,79,XX,XX,XX,*72");
}

TEST(AssemblerInstructions, AND_TableDrivenTest)
{
    test6502_65c02Instruction("and", "29,2D,25,XX,21,31,35,^39,3D,39,XX,XX,XX,*32");
}

TEST(AssemblerInstructions, ASL_TableDrivenTest)
{
    test6502_65c02Instruction("asl", "XX,0E,06,0A,XX,XX,16,XX,1E,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, BCC_AllAddressingMode)
{
    test6502RelativeBranchInstruction("bcc", 0x90);
}

TEST(AssemblerInstructions, BCS_AllAddressingMode)
{
    test6502RelativeBranchInstruction("bcs", 0xB0);
}

TEST(AssemblerInstructions, BEQ_AllAddressingModes)
{
    test6502RelativeBranchInstruction("beq", 0xF0);
}

TEST(AssemblerInstructions, BIT_TableDrivenTest)
{
    test6502_65c02Instruction("bit", "*89,2C,24,XX,XX,XX,*34,XX,*3C,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, BMI_AllAddressingMode)
{
    test6502RelativeBranchInstruction("bmi", 0x30);
}

TEST(AssemblerInstructions, BNE_AllAddressingMode)
{
    test6502RelativeBranchInstruction("bne", 0xD0);
}

TEST(AssemblerInstructions, BPL_AllAddressingMode)
{
    test6502RelativeBranchInstruction("bpl", 0x10);
}

TEST(AssemblerInstructions, BRA_AllAddressingMode)
{
    m_switchTo65c02 = FALSE;
    m_isInvalidInstruction = TRUE;
    test6502RelativeBranchInstruction("bra", 0x80);

    m_switchTo65c02 = TRUE;
    m_isInvalidInstruction = FALSE;
    test6502RelativeBranchInstruction("bra", 0x80);
}

TEST(AssemblerInstructions, BRK_TableDrivenTest)
{
    test6502_65c02Instruction("brk", "XX,XX,XX,00,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, BVC_AllAddressingMode)
{
    test6502RelativeBranchInstruction("bvc", 0x50);
}

TEST(AssemblerInstructions, BVS_AllAddressingMode)
{
    test6502RelativeBranchInstruction("bvs", 0x70);
}

TEST(AssemblerInstructions, CLC_TableDrivenTest)
{
    test6502_65c02Instruction("clc", "XX,XX,XX,18,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, CLD_TableDrivenTest)
{
    test6502_65c02Instruction("cld", "XX,XX,XX,D8,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, CLI_TableDrivenTest)
{
    test6502_65c02Instruction("cli", "XX,XX,XX,58,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, CLV_TableDrivenTest)
{
    test6502_65c02Instruction("clv", "XX,XX,XX,B8,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, CMP_TableDrivenTest)
{
    test6502_65c02Instruction("cmp", "C9,CD,C5,XX,C1,D1,D5,^D9,DD,D9,XX,XX,XX,*D2");
}

TEST(AssemblerInstructions, CPX_TableDrivenTest)
{
    test6502_65c02Instruction("cpx", "E0,EC,E4,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, CPY_TableDrivenTest)
{
    test6502_65c02Instruction("cpy", "C0,CC,C4,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, DEA_TableDrivenTest)
{
    test65c02OnlyInstruction("dea", "XX,XX,XX,3A,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, DEC_TableDrivenTest)
{
    test6502_65c02Instruction("dec", "XX,CE,C6,XX,XX,XX,D6,XX,DE,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, DEX_TableDrivenTest)
{
    test6502_65c02Instruction("dex", "XX,XX,XX,CA,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, DEY_TableDrivenTest)
{
    test6502_65c02Instruction("dey", "XX,XX,XX,88,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, EOR_TableDrivenTest)
{
    test6502_65c02Instruction("eor", "49,4D,45,XX,41,51,55,^59,5D,59,XX,XX,XX,*52");
}

TEST(AssemblerInstructions, INA_TableDrivenTest)
{
    test65c02OnlyInstruction("ina", "XX,XX,XX,1A,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, INC_TableDrivenTest)
{
    test6502_65c02Instruction("inc", "XX,EE,E6,XX,XX,XX,F6,XX,FE,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, INX_TableDrivenTest)
{
    test6502_65c02Instruction("inx", "XX,XX,XX,E8,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, INY_TableDrivenTest)
{
    test6502_65c02Instruction("iny", "XX,XX,XX,C8,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, JMP_TableDrivenTest)
{
    test6502_65c02Instruction("jmp", "XX,4C,^4C,XX,^*7C,XX,XX,XX,XX,XX,XX,6C,*7C,^6C");
}

TEST(AssemblerInstructions, JSR_TableDrivenTest)
{
    test6502_65c02Instruction("jsr", "XX,20,^20,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, LDA_TableDrivenTest)
{
    test6502_65c02Instruction("lda", "A9,AD,A5,XX,A1,B1,B5,^B9,BD,B9,XX,XX,XX,*B2");
}

TEST(AssemblerInstructions, LDX_TableDrivenTest)
{
    test6502_65c02Instruction("ldx", "A2,AE,A6,XX,XX,XX,XX,B6,XX,BE,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, LDY_TableDrivenTest)
{
    test6502_65c02Instruction("ldy", "A0,AC,A4,XX,XX,XX,B4,XX,BC,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, LSR_TableDrivenTest)
{
    test6502_65c02Instruction("lsr", "XX,4E,46,4A,XX,XX,56,XX,5E,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, NOP_TableDrivenTest)
{
    test6502_65c02Instruction("nop", "XX,XX,XX,EA,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, ORA_TableDrivenTest)
{
    test6502_65c02Instruction("ora", "09,0D,05,XX,01,11,15,^19,1D,19,XX,XX,XX,*12");
}

TEST(AssemblerInstructions, PHA_TableDrivenTest)
{
    test6502_65c02Instruction("pha", "XX,XX,XX,48,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, PHP_TableDrivenTest)
{
    test6502_65c02Instruction("php", "XX,XX,XX,08,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, PHX_TableDrivenTest)
{
    test65c02OnlyInstruction("phx", "XX,XX,XX,DA,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, PHY_TableDrivenTest)
{
    test65c02OnlyInstruction("phy", "XX,XX,XX,5A,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, PLA_TableDrivenTest)
{
    test6502_65c02Instruction("pla", "XX,XX,XX,68,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, PLP_TableDrivenTest)
{
    test6502_65c02Instruction("plp", "XX,XX,XX,28,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, PLX_TableDrivenTest)
{
    test65c02OnlyInstruction("plx", "XX,XX,XX,FA,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, PLY_TableDrivenTest)
{
    test65c02OnlyInstruction("ply", "XX,XX,XX,7A,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, ROL_TableDrivenTest)
{
    test6502_65c02Instruction("rol", "XX,2E,26,2A,XX,XX,36,XX,3E,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, ROR_TableDrivenTest)
{
    test6502_65c02Instruction("ror", "XX,6E,66,6A,XX,XX,76,XX,7E,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, RTI_TableDrivenTest)
{
    test6502_65c02Instruction("rti", "XX,XX,XX,40,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, RTS_TableDrivenTest)
{
    test6502_65c02Instruction("rts", "XX,XX,XX,60,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, SBC_TableDrivenTest)
{
    test6502_65c02Instruction("sbc", "E9,ED,E5,XX,E1,F1,F5,^F9,FD,F9,XX,XX,XX,*F2");
}

TEST(AssemblerInstructions, SEC_TableDrivenTest)
{
    test6502_65c02Instruction("sec", "XX,XX,XX,38,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, SED_TableDrivenTest)
{
    test6502_65c02Instruction("sed", "XX,XX,XX,F8,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, SEI_TableDrivenTest)
{
    test6502_65c02Instruction("sei", "XX,XX,XX,78,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, STA_TableDrivenTest)
{
    test6502_65c02Instruction("sta", "XX,8D,85,XX,81,91,95,^99,9D,99,XX,XX,XX,*92");
}

TEST(AssemblerInstructions, STX_TableDrivenTest)
{
    test6502_65c02Instruction("stx", "XX,8E,86,XX,XX,XX,XX,96,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, STY_TableDrivenTest)
{
    test6502_65c02Instruction("sty", "XX,8C,84,XX,XX,XX,94,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, STZ_TableDrivenTest)
{
    test65c02OnlyInstruction("stz", "XX,9C,64,XX,XX,XX,74,XX,9E,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, TAX_TableDrivenTest)
{
    test6502_65c02Instruction("tax", "XX,XX,XX,AA,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, TAY_TableDrivenTest)
{
    test6502_65c02Instruction("tay", "XX,XX,XX,A8,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, TRB_TableDrivenTest)
{
    test65c02OnlyInstruction("trb", "XX,1C,14,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, TSB_TableDrivenTest)
{
    test65c02OnlyInstruction("tsb", "XX,0C,04,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, TSX_TableDrivenTest)
{
    test6502_65c02Instruction("tsx", "XX,XX,XX,BA,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, TXA_TableDrivenTest)
{
    test6502_65c02Instruction("txa", "XX,XX,XX,8A,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, TXS_TableDrivenTest)
{
    test6502_65c02Instruction("txs", "XX,XX,XX,9A,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}

TEST(AssemblerInstructions, TYA_TableDrivenTest)
{
    test6502_65c02Instruction("tya", "XX,XX,XX,98,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX");
}
