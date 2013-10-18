/*  Copyright (C) 2013  Adam Green (https://github.com/adamgreen)
    Copyright (C) 2013  Tee-Kiah Chia

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


/*  Used for unsupported addressing modes in opcode table entries.

    I choose the MVP opcode for this purpose, since it requires special
    handling anyway (if implemented).  -- tkchia 20131010
*/
#define _xXX 0x44

/*  Used in the opcodeZeroPage... fields to indicate that the corresponding
    absolute (direct/indirect/indexed) operation uses long addressing mode. 
    I choose the MVN opcode for this purpose.  -- tkchia 20131010
*/
#define _xLL 0x54


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
static void handleMAC(Assembler* pThis);
static void handleMACend(Assembler* pThis);
static void handleORG(Assembler* pThis);
static void handlePUT(Assembler* pThis);
static void handleREV(Assembler* pThis);
static void handleSAV(Assembler* pThis);
static void handleUSR(Assembler* pThis);
static void handleXC(Assembler* pThis);
static void handleMX(Assembler* pThis);
static void handleREP(Assembler* pThis);
static void handleSEP(Assembler* pThis);
static void handleXCE(Assembler* pThis);
static void handleMVN(Assembler* pThis);
static void handleMVP(Assembler* pThis);
static void ignoreOperator(Assembler* pThis);


static const OpCodeEntry g_6502InstructionSet[] =
{
    /* Assembler Directives */
    {"--^",  handleLUPend,   _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"<<<",  handleMACend,   _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"=",    handleEQU,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"ASC",  handleASC,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"DA",   handleDA,       _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"DB",   handleDB,       _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"DEND", handleDEND,     _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"DFB",  handleDB,       _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"DO",   handleDO,       _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"DS",   handleDS,       _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"DW",   handleDA,       _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"DUM",  handleDUM,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"ELSE", handleELSE,     _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"EQU",  handleEQU,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"FIN",  handleFIN,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"LST",  ignoreOperator, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"LSTDO",ignoreOperator, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"MX",   ignoreOperator, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"HEX",  handleHEX,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"LUP",  handleLUP,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"MAC",  handleMAC,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"ORG",  handleORG,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"PUT",  handlePUT,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"REV",  handleREV,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"SAV",  handleSAV,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"TR",   ignoreOperator, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"USR",  handleUSR,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"XC",   handleXC,       _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    
    /* 6502 Instructions */
    {"ADC", NULL, 0x69, 0x6D, 0x65, _xXX, 0x61, 0x71, 0x75, _xXX, 0x7D, 0x79, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"AND", NULL, 0x29, 0x2D, 0x25, _xXX, 0x21, 0x31, 0x35, _xXX, 0x3D, 0x39, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"ASL", NULL, _xXX, 0x0E, 0x06, 0x0A, _xXX, _xXX, 0x16, _xXX, 0x1E, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"BCC", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x90, _xXX, _xXX, _xXX, 0, 0},
    {"BCS", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0xB0, _xXX, _xXX, _xXX, 0, 0},
    {"BEQ", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0xF0, _xXX, _xXX, _xXX, 0, 0},
    {"BGE", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0xB0, _xXX, _xXX, _xXX, 0, 0},
    {"BIT", NULL, _xXX, 0x2C, 0x24, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"BLT", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x90, _xXX, _xXX, _xXX, 0, 0},
    {"BMI", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x30, _xXX, _xXX, _xXX, 0, 0},
    {"BNE", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0xD0, _xXX, _xXX, _xXX, 0, 0},
    {"BPL", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x10, _xXX, _xXX, _xXX, 0, 0},
    {"BRK", NULL, _xXX, _xXX, _xXX, 0x00, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"BVC", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x50, _xXX, _xXX, _xXX, 0, 0},
    {"BVS", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x70, _xXX, _xXX, _xXX, 0, 0},
    {"CLC", NULL, _xXX, _xXX, _xXX, 0x18, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"CLD", NULL, _xXX, _xXX, _xXX, 0xD8, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"CLI", NULL, _xXX, _xXX, _xXX, 0x58, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"CLV", NULL, _xXX, _xXX, _xXX, 0xB8, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"CMP", NULL, 0xC9, 0xCD, 0xC5, _xXX, 0xC1, 0xD1, 0xD5, _xXX, 0xDD, 0xD9, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"CPX", NULL, 0xE0, 0xEC, 0xE4, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"CPY", NULL, 0xC0, 0xCC, 0xC4, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"DEC", NULL, _xXX, 0xCE, 0xC6, _xXX, _xXX, _xXX, 0xD6, _xXX, 0xDE, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"DEX", NULL, _xXX, _xXX, _xXX, 0xCA, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"DEY", NULL, _xXX, _xXX, _xXX, 0x88, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"EOR", NULL, 0x49, 0x4D, 0x45, _xXX, 0x41, 0x51, 0x55, _xXX, 0x5D, 0x59, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"INC", NULL, _xXX, 0xEE, 0xE6, _xXX, _xXX, _xXX, 0xF6, _xXX, 0xFE, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"INX", NULL, _xXX, _xXX, _xXX, 0xE8, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"INY", NULL, _xXX, _xXX, _xXX, 0xC8, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"JMP", NULL, _xXX, 0x4C, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x6C, _xXX, _xXX, 0, 0},
    {"JSR", NULL, _xXX, 0x20, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"LDA", NULL, 0xA9, 0xAD, 0xA5, _xXX, 0xA1, 0xB1, 0xB5, _xXX, 0xBD, 0xB9, _xXX, _xXX, _xXX, _xXX, 1, 0},
    {"LDX", NULL, 0xA2, 0xAE, 0xA6, _xXX, _xXX, _xXX, _xXX, 0xB6, _xXX, 0xBE, _xXX, _xXX, _xXX, _xXX, 0, 1},
    {"LDY", NULL, 0xA0, 0xAC, 0xA4, _xXX, _xXX, _xXX, 0xB4, _xXX, 0xBC, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 1},
    {"LSR", NULL, _xXX, 0x4E, 0x46, 0x4A, _xXX, _xXX, 0x56, _xXX, 0x5E, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"NOP", NULL, _xXX, _xXX, _xXX, 0xEA, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"ORA", NULL, 0x09, 0x0D, 0x05, _xXX, 0x01, 0x11, 0x15, _xXX, 0x1D, 0x19, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"PHA", NULL, _xXX, _xXX, _xXX, 0x48, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"PHP", NULL, _xXX, _xXX, _xXX, 0x08, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"PLA", NULL, _xXX, _xXX, _xXX, 0x68, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"PLP", NULL, _xXX, _xXX, _xXX, 0x28, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"ROL", NULL, _xXX, 0x2E, 0x26, 0x2A, _xXX, _xXX, 0x36, _xXX, 0x3E, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"ROR", NULL, _xXX, 0x6E, 0x66, 0x6A, _xXX, _xXX, 0x76, _xXX, 0x7E, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"RTI", NULL, _xXX, _xXX, _xXX, 0x40, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"RTS", NULL, _xXX, _xXX, _xXX, 0x60, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"SBC", NULL, 0xE9, 0xED, 0xE5, _xXX, 0xE1, 0xF1, 0xF5, _xXX, 0xFD, 0xF9, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"SEC", NULL, _xXX, _xXX, _xXX, 0x38, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"SED", NULL, _xXX, _xXX, _xXX, 0xF8, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"SEI", NULL, _xXX, _xXX, _xXX, 0x78, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"STA", NULL, _xXX, 0x8D, 0x85, _xXX, 0x81, 0x91, 0x95, _xXX, 0x9D, 0x99, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"STX", NULL, _xXX, 0x8E, 0x86, _xXX, _xXX, _xXX, _xXX, 0x96, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"STY", NULL, _xXX, 0x8C, 0x84, _xXX, _xXX, _xXX, 0x94, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"TAX", NULL, _xXX, _xXX, _xXX, 0xAA, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"TAY", NULL, _xXX, _xXX, _xXX, 0xA8, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"TSX", NULL, _xXX, _xXX, _xXX, 0xBA, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"TXA", NULL, _xXX, _xXX, _xXX, 0x8A, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"TXS", NULL, _xXX, _xXX, _xXX, 0x9A, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"TYA", NULL, _xXX, _xXX, _xXX, 0x98, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0}
};

static const OpCodeEntry g_65c02AdditionalInstructions[] =
{
    /* 65c02 Instructions */
    {"ADC", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x72, 0, 0},
    {"AND", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x32, 0, 0},
    {"BIT", NULL, 0x89, _xXX, _xXX, _xXX, _xXX, _xXX, 0x34, _xXX, 0x3C, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"BRA", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x80, _xXX, _xXX, _xXX, 0, 0},
    {"CMP", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0xD2, 0, 0},
    {"DEA", NULL, _xXX, _xXX, _xXX, 0x3A, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"EOR", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x52, 0, 0},
    {"INA", NULL, _xXX, _xXX, _xXX, 0x1A, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"JMP", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x7C, _xXX, 0, 0},
    {"LDA", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0xB2, 1, 0},
    {"ORA", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x12, 0, 0},
    {"PHX", NULL, _xXX, _xXX, _xXX, 0xDA, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"PHY", NULL, _xXX, _xXX, _xXX, 0x5A, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"PLX", NULL, _xXX, _xXX, _xXX, 0xFA, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"PLY", NULL, _xXX, _xXX, _xXX, 0x7A, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"SBC", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0xF2, 0, 0},
    {"STA", NULL, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0x92, 0, 0},
    {"STZ", NULL, _xXX, 0x9C, 0x64, _xXX, _xXX, _xXX, 0x74, _xXX, 0x9E, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"TRB", NULL, _xXX, 0x1C, 0x14, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"TSB", NULL, _xXX, 0x0C, 0x04, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
};

static const OpCodeEntry g_65816AdditionalInstructions[] =
{
    /* Assembler Directives */
    {"MX",   handleMX,       _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},

    /* 65816 Instructions That Are A Bit Like Directives (they change the assembler's interpretation of subsequent
       instructions)
    */
    {"REP",  handleREP,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"SEP",  handleSEP,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"XCE",  handleXCE,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},

    /* 65816 Instructions Handled Specially */
    {"MVN",  handleMVN,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"MVP",  handleMVP,      _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},

    /* Other 65816 Instructions */
    {"DEC", NULL, _xXX, _xXX, _xXX, 0x3A, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"INC", NULL, _xXX, _xXX, _xXX, 0x1A, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"LDAL",NULL, _xXX, 0xAF, _xLL, _xXX, _xLL, _xXX, _xLL, _xLL, 0xBF, _xXX, _xXX, _xXX, _xXX, _xLL, 0, 0},
    {"PHB", NULL, _xXX, _xXX, _xXX, 0x8B, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"PLB", NULL, _xXX, _xXX, _xXX, 0xAB, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, _xXX, 0, 0},
    {"STAL",NULL, _xXX, 0x8F, _xLL, _xXX, _xLL, _xXX, _xLL, _xLL, 0x9F, _xXX, _xXX, _xXX, _xXX, _xLL, 0, 0},
};

#endif /* _INSTRUCTION_SETS_H_ */
