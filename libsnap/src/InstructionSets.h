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
#ifndef _INSTRUCTION_SETS_H_
#define _INSTRUCTION_SETS_H_

#include "AssemblerPriv.h"


/* Used for unsupported addressing modes in opcode table entries. */
#define _xXX 0xFF


/* Forward declaration of directive handling routines. */
static void handleASC(Assembler* pThis);
static void handleDA(Assembler* pThis);
static void handleDB(Assembler* pThis);
static void handleDEND(Assembler* pThis);
static void handleDO(Assembler* pThis);
static void handleDS(Assembler* pThis);
static void handleDUM(Assembler* pThis);
static void handleELSE(Assembler* pThis);
static void handleEQU(Assembler* pThis);
static void handleFIN(Assembler* pThis);
static void handleHEX(Assembler* pThis);
static void handleLUP(Assembler* pThis);
static void handleLUPend(Assembler* pThis);
static void handleORG(Assembler* pThis);
static void handlePUT(Assembler* pThis);
static void handleREV(Assembler* pThis);
static void handleSAV(Assembler* pThis);
static void handleUSR(Assembler* pThis);
static void handleXC(Assembler* pThis);
static void ignoreOperator(Assembler* pThis);


static const OpCodeEntry g_6502InstructionSet[] =
{
    /* Assembler Directives */
    {"--^",  handleLUPend,   _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"=",    handleEQU,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"ASC",  handleASC,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"DA",   handleDA,       _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"DB",   handleDB,       _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"DEND", handleDEND,     _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"DFB",  handleDB,       _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"DO",   handleDO,       _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"DS",   handleDS,       _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"DW",   handleDA,       _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"DUM",  handleDUM,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"ELSE", handleELSE,     _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"EQU",  handleEQU,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"FIN",  handleFIN,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"LST",  ignoreOperator, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"LSTDO",ignoreOperator, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"MX",   ignoreOperator, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"HEX",  handleHEX,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"LUP",  handleLUP,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"ORG",  handleORG,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"PUT",  handlePUT,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"REV",  handleREV,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"SAV",  handleSAV,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"TR",   ignoreOperator, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"USR",  handleUSR,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"XC",   handleXC,       _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    
    /* 6502 Instructions */
    {"ADC", NULL, 0x69, 0x6D, 0x65, _xXX, 0x61, 0x71, 0x75, _xXX, 0x7D, 0x79, _xXX, _xXX, _xXX, _xXX},
    {"AND", NULL, 0x29, 0x2D, 0x25, _xXX, 0x21, 0x31, 0x35, _xXX, 0x3D, 0x39, _xXX, _xXX, _xXX, _xXX},
    {"ASL", NULL, _xXX, 0x0E, 0x06, 0x0A, _xXX, _xXX, 0x16, _xXX, 0x1E, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"BCC", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x90, _xXX, _xXX, _xXX},
    {"BCS", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0xB0, _xXX, _xXX, _xXX},
    {"BEQ", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0xF0, _xXX, _xXX, _xXX},
    {"BIT", NULL, _xXX, 0x2C, 0x24, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"BMI", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x30, _xXX, _xXX, _xXX},
    {"BNE", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0xD0, _xXX, _xXX, _xXX},
    {"BPL", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x10, _xXX, _xXX, _xXX},
    {"BRK", NULL, _xXX, _xXX, _xXX, 0x00, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"BVC", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x50, _xXX, _xXX, _xXX},
    {"BVS", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x70, _xXX, _xXX, _xXX},
    {"CLC", NULL, _xXX, _xXX, _xXX, 0x18, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"CLD", NULL, _xXX, _xXX, _xXX, 0xD8, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"CLI", NULL, _xXX, _xXX, _xXX, 0x58, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"CLV", NULL, _xXX, _xXX, _xXX, 0xB8, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"CMP", NULL, 0xC9, 0xCD, 0xC5, _xXX, 0xC1, 0xD1, 0xD5, _xXX, 0xDD, 0xD9, _xXX, _xXX, _xXX, _xXX},
    {"CPX", NULL, 0xE0, 0xEC, 0xE4, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"CPY", NULL, 0xC0, 0xCC, 0xC4, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"DEC", NULL, _xXX, 0xCE, 0xC6, _xXX, _xXX, _xXX, 0xD6, _xXX, 0xDE, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"DEX", NULL, _xXX, _xXX, _xXX, 0xCA, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"DEY", NULL, _xXX, _xXX, _xXX, 0x88, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"EOR", NULL, 0x49, 0x4D, 0x45, _xXX, 0x41, 0x51, 0x55, _xXX, 0x5D, 0x59, _xXX, _xXX, _xXX, _xXX},
    {"INC", NULL, _xXX, 0xEE, 0xE6, _xXX, _xXX, _xXX, 0xF6, _xXX, 0xFE, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"INX", NULL, _xXX, _xXX, _xXX, 0xE8, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"INY", NULL, _xXX, _xXX, _xXX, 0xC8, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"JMP", NULL, _xXX, 0x4C, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x6C, _xXX, _xXX},
    {"JSR", NULL, _xXX, 0x20, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"LDA", NULL, 0xA9, 0xAD, 0xA5, _xXX, 0xA1, 0xB1, 0xB5, _xXX, 0xBD, 0xB9, _xXX, _xXX, _xXX, _xXX},
    {"LDX", NULL, 0xA2, 0xAE, 0xA6, _xXX, _xXX, _xXX, _xXX, 0xB6, _xXX, 0xBE, _xXX, _xXX, _xXX, _xXX},
    {"LDY", NULL, 0xA0, 0xAC, 0xA4, _xXX, _xXX, _xXX, 0xB4, _xXX, 0xBC, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"LSR", NULL, _xXX, 0x4E, 0x46, 0x4A, _xXX, _xXX, 0x56, _xXX, 0x5E, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"NOP", NULL, _xXX, _xXX, _xXX, 0xEA, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"ORA", NULL, 0x09, 0x0D, 0x05, _xXX, 0x01, 0x11, 0x15, _xXX, 0x1D, 0x19, _xXX, _xXX, _xXX, _xXX},
    {"PHA", NULL, _xXX, _xXX, _xXX, 0x48, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"PHP", NULL, _xXX, _xXX, _xXX, 0x08, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"PLA", NULL, _xXX, _xXX, _xXX, 0x68, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"PLP", NULL, _xXX, _xXX, _xXX, 0x28, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"ROL", NULL, _xXX, 0x2E, 0x26, 0x2A, _xXX, _xXX, 0x36, _xXX, 0x3E, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"ROR", NULL, _xXX, 0x6E, 0x66, 0x6A, _xXX, _xXX, 0x76, _xXX, 0x7E, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"RTI", NULL, _xXX, _xXX, _xXX, 0x40, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"RTS", NULL, _xXX, _xXX, _xXX, 0x60, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"SBC", NULL, 0xE9, 0xED, 0xE5, _xXX, 0xE1, 0xF1, 0xF5, _xXX, 0xFD, 0xF9, _xXX, _xXX, _xXX, _xXX},
    {"SEC", NULL, _xXX, _xXX, _xXX, 0x38, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"SED", NULL, _xXX, _xXX, _xXX, 0xF8, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"SEI", NULL, _xXX, _xXX, _xXX, 0x78, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"STA", NULL, _xXX, 0x8D, 0x85, _xXX, 0x81, 0x91, 0x95, _xXX, 0x9D, 0x99, _xXX, _xXX, _xXX, _xXX},
    {"STX", NULL, _xXX, 0x8E, 0x86, _xXX, _xXX, _xXX, _xXX, 0x96, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"STY", NULL, _xXX, 0x8C, 0x84, _xXX, _xXX, _xXX, 0x94, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"TAX", NULL, _xXX, _xXX, _xXX, 0xAA, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"TAY", NULL, _xXX, _xXX, _xXX, 0xA8, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"TSX", NULL, _xXX, _xXX, _xXX, 0xBA, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"TXA", NULL, _xXX, _xXX, _xXX, 0x8A, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"TXS", NULL, _xXX, _xXX, _xXX, 0x9A, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"TYA", NULL, _xXX, _xXX, _xXX, 0x98, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX}
};

static const OpCodeEntry g_65c02AdditionalInstructions[] =
{
    /* 65c02 Instructions */
    {"ADC", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x72},
    {"AND", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x32},
    {"BIT", NULL, 0x89, _xXX, _xXX, _xXX, _xXX, _xXX, 0x34, _xXX, 0x3C, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"BRA", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x80, _xXX, _xXX, _xXX},
    {"CMP", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0xD2},
    {"DEA", NULL, _xXX, _xXX, _xXX, 0x3A, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"EOR", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x52},
    {"INA", NULL, _xXX, _xXX, _xXX, 0x1A, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"JMP", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x7C, _xXX},
    {"LDA", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0xB2},
    {"ORA", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x12},
    {"PHX", NULL, _xXX, _xXX, _xXX, 0xDA, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"PHY", NULL, _xXX, _xXX, _xXX, 0x5A, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"PLX", NULL, _xXX, _xXX, _xXX, 0xFA, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"PLY", NULL, _xXX, _xXX, _xXX, 0x7A, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"SBC", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0xF2},
    {"STA", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x92},
    {"STZ", NULL, _xXX, 0x9C, 0x64, _xXX, _xXX, _xXX, 0x74, _xXX, 0x9E, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"TRB", NULL, _xXX, 0x1C, 0x14, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
    {"TSB", NULL, _xXX, 0x0C, 0x04, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX},
};

#endif /* _INSTRUCTION_SETS_H_ */
