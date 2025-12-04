#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <cstring>
#include <iostream>

#include "bus.h"
#include "cpu.h"
#include "ppu.h"
#include "cpu_vars.h"
// #include "tracer.h"

#define DEBUG

using std::cout;
typedef void (*cpu_op_fn)(uint16_t address);
typedef void (*cpu_op_fn_acc)();
const uint16_t null_address = 0;

// All instructions are explained here: https://www.masswerk.at/6502/6502_instruction_set.html
// For a better understanding of how the CPU works, check this awesome resource: https://www.nesdev.org/obelisk-6502-guide/

void OP_NOOP() {};
void OP_BRK() { BRK(); };
void OP_BPL()
{
    int8_t branch_position = (int8_t)read_pc();
    if (N == 0)
        PC += branch_position;
};

void OP_JSR()
{
    uint16_t address = read_abs_address(PC);
    PC += 1;
    JSR_abs(address);
}

void OP_BMI()
{
    int8_t branch_position = (int8_t)read_pc();
    if (N == 1)
        PC += branch_position;
};
void OP_RTI() { RTI(); };
void OP_BVC()
{
    int8_t branch_position = (int8_t)read_pc();
    if (O == 0)
        PC += branch_position;
};
void OP_RTS() { RTS(); };
void OP_BVS()
{
    int8_t branch_position = (int8_t)read_pc();
    if (O == 1)
        PC += branch_position;
};
void OP_BCC()
{
    int8_t branch_position = (int8_t)read_pc();
    if (C == 0)
        PC += branch_position;
};
void OP_LDY_IMM() { LDY(addr_IMM()); };
void OP_BCS()
{
    int8_t branch_position = (int8_t)read_pc();
    if (C == 1)
        PC += branch_position;
};
void OP_CPY_IMM() { CPY(addr_IMM()); };
void OP_BNE()
{
    int8_t branch_position = (int8_t)read_pc();
    if (Z == 0)
        PC += branch_position;
};
void OP_CPX_IMM() { CPX(addr_IMM()); };
void OP_BEQ()
{
    int8_t branch_position = (int8_t)read_pc();
    if (Z == 1)
        PC += branch_position;
};
void OP_ORA_ZP_X_P() { ORA(addr_zero_page_x_p()); };
void OP_ORA_ZP_P_Y() { ORA(addr_pzero_page_y()); };
void OP_AND_ZP_X_P() { AND(addr_zero_page_x_p()); };
void OP_AND_ZP_P_Y() { AND(addr_pzero_page_y()); };
void OP_EOR_ZP_X_P() { EOR(addr_zero_page_x_p()); };
void OP_EOR_ZP_P_Y() { EOR(addr_pzero_page_y()); };
void OP_ADC_ZP_X_P() { ADC(addr_zero_page_x_p()); };
void OP_ADC_ZP_P_Y() { ADC(addr_pzero_page_y()); };
void OP_STA_ZP_X_P() { STA(addr_zero_page_x_p()); };
void OP_STA_ZP_P_Y() { STA(addr_pzero_page_y()); };
void OP_LDA_ZP_X_P() { LDA(addr_zero_page_x_p()); };
void OP_LDA_ZP_P_Y() { LDA(addr_pzero_page_y()); };
void OP_CMP_ZP_X_P() { CMP(addr_zero_page_x_p()); };
void OP_CMP_ZP_P_Y() { CMP(addr_pzero_page_y()); };
void OP_SBC_ZP_X_P() { SBC(addr_zero_page_x_p()); };
void OP_SBC_ZP_P_Y() { SBC(addr_pzero_page_y()); };
void OP_LDX_IMM() { LDX(addr_IMM()); };
void OP_BIT_ZP() { BITT(addr_zero_page()); };
void OP_STY_ZP() { STY(addr_zero_page()); };
void OP_STY_ZP_X() { STY(addr_zero_page_x()); };
void OP_LDY_ZP() { LDY(addr_zero_page()); };
void OP_LDY_ZP_X() { LDY(addr_zero_page_x()); };
void OP_CPY_ZP() { CPY(addr_zero_page()); };
void OP_CPX_ZP() { CPX(addr_zero_page()); };
void OP_ORA_ZP() { ORA(addr_zero_page()); };
void OP_ORA_ZP_X() { ORA(addr_zero_page_x()); };
void OP_AND_ZP() { AND(addr_zero_page()); };
void OP_AND_ZP_X() { AND(addr_zero_page_x()); };
void OP_EOR_ZP() { EOR(addr_zero_page()); };
void OP_EOR_ZP_X() { EOR(addr_zero_page_x()); };
void OP_ADC_ZP() { ADC(addr_zero_page()); };
void OP_ADC_ZP_X() { ADC(addr_zero_page_x()); };
void OP_STA_ZP() { STA(addr_zero_page()); };
void OP_STA_ZP_X() { STA(addr_zero_page_x()); };
void OP_LDA_ZP() { LDA(addr_zero_page()); };
void OP_LDA_ZP_X() { LDA(addr_zero_page_x()); };
void OP_CMP_ZP() { CMP(addr_zero_page()); };
void OP_CMP_ZP_X() { CMP(addr_zero_page_x()); };
void OP_SBC_ZP() { SBC(addr_zero_page()); };
void OP_SBC_ZP_X() { SBC(addr_zero_page_x()); };
void OP_ASL_ZP() { ASL(addr_zero_page()); };
void OP_ASL_ZP_X() { ASL(addr_zero_page_x()); };
void OP_ROL_ZP() { ROL(addr_zero_page()); };
void OP_ROL_ZP_X() { ROL(addr_zero_page_x()); };
void OP_LSR_ZP() { LSR(addr_zero_page()); };
void OP_LSR_ZP_X() { LSR(addr_zero_page_x()); };
void OP_ROR_ZP() { ROR(addr_zero_page()); };
void OP_ROR_ZP_X() { ROR(addr_zero_page_x()); };
void OP_STX_ZP() { STX(addr_zero_page()); };
void OP_STX_ZP_Y() { STX(addr_zero_page_y()); };
void OP_LDX_ZP() { LDX(addr_zero_page()); };
void OP_LDX_ZP_Y() { LDX(addr_zero_page_y()); };
void OP_DEC_ZP() { DECC(addr_zero_page()); };
void OP_DEC_ZP_X() { DECC(addr_zero_page_x()); };
void OP_INC_ZP() { INC(addr_zero_page()); };
void OP_INC_ZP_X() { INC(addr_zero_page_x()); };
void OP_ORA_IMM() { ORA(addr_IMM()); };
void OP_ORA_ABS_Y() { ORA(addr_abs_y()); };
void OP_AND_IMM() { AND(addr_IMM()); };
void OP_AND_ABS_Y() { AND(addr_abs_y()); };
void OP_EOR_IMM() { EOR(addr_IMM()); };
void OP_EOR_ABS_Y() { EOR(addr_abs_y()); };
void OP_ADC_IMM() { ADC(addr_IMM()); };
void OP_ADC_ABS_Y() { ADC(addr_abs_y()); };
void OP_STA_ABS_Y() { STA(addr_abs_y()); };
void OP_LDA_IMM() { LDA(addr_IMM()); };
void OP_LDA_ABS_Y() { LDA(addr_abs_y()); };
void OP_CMP_IMM() { CMP(addr_IMM()); };
void OP_CMP_ABS_Y() { CMP(addr_abs_y()); };
void OP_SBC_IMM() { SBC(addr_IMM()); };
void OP_SBC_ABS_Y() { SBC(addr_abs_y()); };
void OP_ASL_ACC() { ASL_acc(); };
void OP_ROL_ACC() { ROL_acc(); };
void OP_LSR_ACC() { LSR_acc(); };
void OP_ROR_ACC() { ROR_acc(); };
void OP_JMP_IND()
{
    uint16_t jump_address;
    jump_address = read_address_from_pc();
    JMP_indirect(jump_address);
}
void OP_JMP_ABS()
{
    uint16_t jump_address;
    jump_address = read_address_from_pc();
    JMP_abs(jump_address);
}

void OP_BIT_ABS() { BITT(addr_abs()); };
void OP_STY_ABS() { STY(addr_abs()); };
void OP_LDY_ABS() { LDY(addr_abs()); };
void OP_LDY_ABS_X() { LDY(addr_abs_x()); };
void OP_CPY_ABS() { CPY(addr_abs()); };
void OP_CPX_ABS() { CPX(addr_abs()); };
void OP_ORA_ABS() { ORA(addr_abs()); };
void OP_ORA_ABS_X() { ORA(addr_abs_x()); };
void OP_AND_ABS() { AND(addr_abs()); };
void OP_AND_ABS_X() { AND(addr_abs_x()); };
void OP_EOR_ABS() { EOR(addr_abs()); };
void OP_EOR_ABS_X() { EOR(addr_abs_x()); };
void OP_ADC_ABS() { ADC(addr_abs()); };
void OP_ADC_ABS_X() { ADC(addr_abs_x()); };
void OP_STA_ABS() { STA(addr_abs()); };
void OP_STA_ABS_X() { STA(addr_abs_x()); };
void OP_LDA_ABS() { LDA(addr_abs()); };
void OP_LDA_ABS_X() { LDA(addr_abs_x()); };
void OP_CMP_ABS() { CMP(addr_abs()); };
void OP_CMP_ABS_X() { CMP(addr_abs_x()); };
void OP_SBC_ABS() { SBC(addr_abs()); };
void OP_SBC_ABS_X() { SBC(addr_abs_x()); };
void OP_ASL_ABS() { ASL(addr_abs()); };
void OP_ASL_ABS_X() { ASL(addr_abs_x()); };
void OP_ROL_ABS() { ROL(addr_abs()); };
void OP_ROL_ABS_X() { ROL(addr_abs_x()); };
void OP_LSR_ABS() { LSR(addr_abs()); };
void OP_LSR_ABS_X() { LSR(addr_abs_x()); };
void OP_ROR_ABS() { ROR(addr_abs()); };
void OP_ROR_ABS_X() { ROR(addr_abs_x()); };
void OP_STX_ABS() { STX(addr_abs()); };
void OP_LDX_ABS() { LDX(addr_abs()); };
void OP_LDX_ABS_Y() { LDX(addr_abs_y()); };
void OP_DEC_ABS() { DECC(addr_abs()); };
void OP_DEC_ABS_X() { DECC(addr_abs_x()); };
void OP_INC_ABS() { INC(addr_abs()); };
void OP_INC_ABS_X() { INC(addr_abs_x()); };

void init_instruction_handler_lut()
{
    for (int i = 0; i <= 0xFF; i++)
    {
        opcode_table[i] = OP_NOOP;
    }
    opcode_table[0x00] = OP_BRK;
    opcode_table[0x10] = OP_BPL;
    opcode_table[0x20] = OP_JSR;
    opcode_table[0x30] = OP_BMI;
    opcode_table[0x40] = OP_RTI;
    opcode_table[0x50] = OP_BVC;
    opcode_table[0x60] = OP_RTS;
    opcode_table[0x70] = OP_BVS;
    opcode_table[0x80] = OP_NOOP; // unoficial opcode
    opcode_table[0x90] = OP_BCC;
    opcode_table[0xA0] = OP_LDY_IMM;
    opcode_table[0xB0] = OP_BCS;
    opcode_table[0xC0] = OP_CPY_IMM;
    opcode_table[0xD0] = OP_BNE;
    opcode_table[0xE0] = OP_CPX_IMM;
    opcode_table[0xF0] = OP_BEQ;
    opcode_table[0x01] = OP_ORA_ZP_X_P;
    opcode_table[0x11] = OP_ORA_ZP_P_Y;
    opcode_table[0x21] = OP_AND_ZP_X_P;
    opcode_table[0x31] = OP_AND_ZP_P_Y;
    opcode_table[0x41] = OP_EOR_ZP_X_P;
    opcode_table[0x51] = OP_EOR_ZP_P_Y;
    opcode_table[0x61] = OP_ADC_ZP_X_P;
    opcode_table[0x71] = OP_ADC_ZP_P_Y;
    opcode_table[0x81] = OP_STA_ZP_X_P;
    opcode_table[0x91] = OP_STA_ZP_P_Y;
    opcode_table[0xA1] = OP_LDA_ZP_X_P;
    opcode_table[0xB1] = OP_LDA_ZP_P_Y;
    opcode_table[0xC1] = OP_CMP_ZP_X_P;
    opcode_table[0xD1] = OP_CMP_ZP_P_Y;
    opcode_table[0xE1] = OP_SBC_ZP_X_P;
    opcode_table[0xF1] = OP_SBC_ZP_P_Y;
    opcode_table[0xA2] = OP_LDX_IMM;
    opcode_table[0x24] = OP_BIT_ZP;
    opcode_table[0x84] = OP_STY_ZP;
    opcode_table[0x94] = OP_STY_ZP_X;
    opcode_table[0xA4] = OP_LDY_ZP;
    opcode_table[0xB4] = OP_LDY_ZP_X;
    opcode_table[0xC4] = OP_CPY_ZP;
    opcode_table[0xE4] = OP_CPX_ZP;
    opcode_table[0x05] = OP_ORA_ZP;
    opcode_table[0x15] = OP_ORA_ZP_X;
    opcode_table[0x25] = OP_AND_ZP;
    opcode_table[0x35] = OP_AND_ZP_X;
    opcode_table[0x45] = OP_EOR_ZP;
    opcode_table[0x55] = OP_EOR_ZP_X;
    opcode_table[0x65] = OP_ADC_ZP;
    opcode_table[0x75] = OP_ADC_ZP_X;
    opcode_table[0x85] = OP_STA_ZP;
    opcode_table[0x95] = OP_STA_ZP_X;
    opcode_table[0xA5] = OP_LDA_ZP;
    opcode_table[0xB5] = OP_LDA_ZP_X;
    opcode_table[0xC5] = OP_CMP_ZP;
    opcode_table[0xD5] = OP_CMP_ZP_X;
    opcode_table[0xE5] = OP_SBC_ZP;
    opcode_table[0xF5] = OP_SBC_ZP_X;
    opcode_table[0x06] = OP_ASL_ZP;
    opcode_table[0x16] = OP_ASL_ZP_X;
    opcode_table[0x26] = OP_ROL_ZP;
    opcode_table[0x36] = OP_ROL_ZP_X;
    opcode_table[0x46] = OP_LSR_ZP;
    opcode_table[0x56] = OP_LSR_ZP_X;
    opcode_table[0x66] = OP_ROR_ZP;
    opcode_table[0x76] = OP_ROR_ZP_X;
    opcode_table[0x86] = OP_STX_ZP;
    opcode_table[0x96] = OP_STX_ZP_Y;
    opcode_table[0xA6] = OP_LDX_ZP;
    opcode_table[0xB6] = OP_LDX_ZP_Y;
    opcode_table[0xC6] = OP_DEC_ZP;
    opcode_table[0xD6] = OP_DEC_ZP_X;
    opcode_table[0xE6] = OP_INC_ZP;
    opcode_table[0xF6] = OP_INC_ZP_X;
    opcode_table[0x08] = PHP;
    opcode_table[0x18] = CLC;
    opcode_table[0x28] = PLP;
    opcode_table[0x38] = SEC;
    opcode_table[0x48] = PHA;
    opcode_table[0x58] = CLI;
    opcode_table[0x68] = PLA;
    opcode_table[0x78] = SEI;
    opcode_table[0x88] = DEY;
    opcode_table[0x98] = TYA;
    opcode_table[0xA8] = TAY;
    opcode_table[0xB8] = CLV;
    opcode_table[0xC8] = INY;
    opcode_table[0xD8] = CLD;
    opcode_table[0xE8] = INX;
    opcode_table[0xF8] = SED;
    opcode_table[0x09] = OP_ORA_IMM;
    opcode_table[0x19] = OP_ORA_ABS_Y;
    opcode_table[0x29] = OP_AND_IMM;
    opcode_table[0x39] = OP_AND_ABS_Y;
    opcode_table[0x49] = OP_EOR_IMM;
    opcode_table[0x59] = OP_EOR_ABS_Y;
    opcode_table[0x69] = OP_ADC_IMM;
    opcode_table[0x79] = OP_ADC_ABS_Y;
    // opcode_table[0x89] = ; NO OPCODE HERE ON CLASSIC 6502
    opcode_table[0x99] = OP_STA_ABS_Y;
    opcode_table[0xA9] = OP_LDA_IMM;
    opcode_table[0xB9] = OP_LDA_ABS_Y;
    opcode_table[0xC9] = OP_CMP_IMM;
    opcode_table[0xD9] = OP_CMP_ABS_Y;
    opcode_table[0xE9] = OP_SBC_IMM;
    opcode_table[0xF9] = OP_SBC_ABS_Y;
    opcode_table[0x0A] = OP_ASL_ACC;
    opcode_table[0x2A] = OP_ROL_ACC;
    opcode_table[0x4A] = OP_LSR_ACC;
    opcode_table[0x6A] = OP_ROR_ACC;
    opcode_table[0x8A] = TXA;
    opcode_table[0x9A] = TXS;
    opcode_table[0xAA] = TAX;
    opcode_table[0xBA] = TSX;
    opcode_table[0xCA] = DEX;
    opcode_table[0xEA] = OP_NOOP;
    opcode_table[0x2C] = OP_BIT_ABS;
    opcode_table[0x4C] = OP_JMP_ABS;
    opcode_table[0x6C] = OP_JMP_IND;
    opcode_table[0x8C] = OP_STY_ABS;
    opcode_table[0xAC] = OP_LDY_ABS;
    opcode_table[0xBC] = OP_LDY_ABS_X;
    opcode_table[0xCC] = OP_CPY_ABS;
    opcode_table[0xEC] = OP_CPX_ABS;
    opcode_table[0x0D] = OP_ORA_ABS;
    opcode_table[0x1D] = OP_ORA_ABS_X;
    opcode_table[0x2D] = OP_AND_ABS;
    opcode_table[0x3D] = OP_AND_ABS_X;
    opcode_table[0x4D] = OP_EOR_ABS;
    opcode_table[0x5D] = OP_EOR_ABS_X;
    opcode_table[0x6D] = OP_ADC_ABS;
    opcode_table[0x7D] = OP_ADC_ABS_X;
    opcode_table[0x8D] = OP_STA_ABS;
    opcode_table[0x9D] = OP_STA_ABS_X;
    opcode_table[0xAD] = OP_LDA_ABS;
    opcode_table[0xBD] = OP_LDA_ABS_X;
    opcode_table[0xCD] = OP_CMP_ABS;
    opcode_table[0xDD] = OP_CMP_ABS_X;
    opcode_table[0xED] = OP_SBC_ABS;
    opcode_table[0xFD] = OP_SBC_ABS_X;
    opcode_table[0x0E] = OP_ASL_ABS;
    opcode_table[0x1E] = OP_ASL_ABS_X;
    opcode_table[0x2E] = OP_ROL_ABS;
    opcode_table[0x3E] = OP_ROL_ABS_X;
    opcode_table[0x4E] = OP_LSR_ABS;
    opcode_table[0x5E] = OP_LSR_ABS_X;
    opcode_table[0x6E] = OP_ROR_ABS;
    opcode_table[0x7E] = OP_ROR_ABS_X;
    opcode_table[0x8E] = OP_STX_ABS;
    // opcode_table[0x9E] = OP_STX_ABS_X;
    opcode_table[0xAE] = OP_LDX_ABS;
    opcode_table[0xBE] = OP_LDX_ABS_Y;
    opcode_table[0xCE] = OP_DEC_ABS;
    opcode_table[0xDE] = OP_DEC_ABS_X;
    opcode_table[0xEE] = OP_INC_ABS;
    opcode_table[0xFE] = OP_INC_ABS_X;
}

// stack opperatons. remember, addresses are 16 bit wide!
void IRAM_ATTR push(byte x)
{
    // Stack overflow should handle itself
    cpu_write(0x0100 + SP, x);
    SP--;
}

//This maybe was the rooot of all my problems?
void push_address(uint16_t address)
{
    cpu_write(0x0100 + SP, (address & 0xFF00) >> 8);
    SP--;
    cpu_write(0x0100 + SP, address & 0x00FF);
    SP--;
}

byte IRAM_ATTR pop()
{
    SP++;
    byte to_return = read(0x0100 + SP);
    return to_return;
}
uint16_t IRAM_ATTR pop_address()
{
    byte low_byte = pop();
    byte high_byte = pop();
    uint16_t to_return = (uint16_t)high_byte << 8;
    to_return |= low_byte;
    return to_return;
}

uint16_t IRAM_ATTR read_address(byte offset)
{
    uint16_t val = read(offset + 1); // little endian
    val <<= 8;
    val |= read(offset);
    return val;
}

// The difference between read_address and read_abs_address is that
// read_abs_address takes a 16bit offset(reads the absolute address)
uint16_t IRAM_ATTR read_abs_address(uint16_t offset)
{
    uint16_t val = read(offset + 1); // little endian
    val <<= 8;
    val |= read(offset);
    return val;
}

uint16_t IRAM_ATTR read_address_from_pc()
{
    uint16_t address = read_abs_address(PC);
    PC += 2;
    return address;
}

// This function runes one opcode, not one cycle. My emulator does not aim to be
// cycle accurate CPU EXECUTE NOW DOES ONLY ONE CLOCK
// This LOOKUP table was obtained by running estimate_duration() from my original emulator for each opcode
// ASsuming a duration of 2 if the opcode doesnt exist
static int DRAM_ATTR OPCODE_duration[256] = {2, 2, 2, 2, 3, 2, 2, 2, 3, 2, 2, 2, 4, 2, 2, 2, 3,
                                             2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6, 2, 2,
                                             2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 4,
                                             2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6, 2, 2, 2, 3, 2, 2,
                                             2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 4, 2, 2, 2, 2,
                                             2, 2, 2, 2, 2, 2, 2, 6, 2, 2, 2, 3, 2, 2, 2, 4, 2, 2,
                                             2, 2, 2, 2, 2, 3, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 2,
                                             2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                             2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                             2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 2,
                                             2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                             2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 4, 2, 2,
                                             2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                             2, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2,
                                             2, 2, 2, 2, 2};

// Maybe switch to a system interrupt when an emulated interrupt occurs, could prevent an if for every iteration - i dont know if it's worth
uint32_t remaining_cycles = 0;
uint32_t debug_count = 0;
void IRAM_ATTR cpu_clock()
{
    if (remaining_cycles)
    {
        remaining_cycles--;
    }
    else
    {
        debug_count++;
        if (!pending_nmi)
        {
            opcode = read_pc();
            if (cpu_debug_print)
            {
                Serial.printf("Opcode: %02X, PC: %04X | A:%02X X:%02X Y:%02X\n", opcode, PC, A, X, Y);
            }
            remaining_cycles += OPCODE_duration[opcode];
            opcode_table[opcode]();
        }
        else
        {
            trigger_nmi();
            remaining_cycles += 7;
            return;
        }
        return; // if it gets here it means it failed
    }
}
