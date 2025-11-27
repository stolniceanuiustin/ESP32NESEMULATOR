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




// stack opperatons. remember, addresses are 16 bit wide!
void IRAM_ATTR push(byte x)
{
    // Stack overflow should handle itself
    cpu_write(0x0100 + SP, x);
    SP--;
}

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


Instruction decode_table[256];
byte high_nibble_table[256];
byte low_nibble_table[256];
byte last_5_bits_table[256];

byte *branch_flag_table[4] = {
    &N,
    &O,
    &C,
    &Z};

void build_decode_table()
{
    for (int op = 0; op < 256; op++)
    {
        decode_table[op].aaa = (op >> 5) & 0x07;
        decode_table[op].bbb = (op >> 2) & 0x07;
        decode_table[op].cc = op & 0x03;
        decode_table[op].xx = op >> 6;
        decode_table[op].y = (op >> 5) & 1;
        decode_table[op].opcode = op;

        last_5_bits_table[op] = (0b00011111 & op);
        low_nibble_table[op] = op & 0x0F;
        high_nibble_table[op] = op >> 4;
    }
}

// i dont know why formating this is so weird?
static const cpu_op_fn group1_table[8] =
    {
        ORA,
        AND,
        EOR,
        ADC,
        STA,
        LDA,
        CMP,
        SBC};

static const cpu_op_fn group2_table_noacc[8] =
    {
        ASL,
        ROL,
        LSR,
        ROR,
        STX,
        LDX,
        DECC,
        INC};

static const cpu_op_fn_acc group2_table_acc[4] = {
    ASL_acc,
    ROL_acc,
    LSR_acc,
    ROR_acc};

void IRAM_ATTR run_instruction_group2(uint16_t address, bool accumulator)
{
    switch (inst.aaa)
    {
    case 0x0:
        ASL_b(address, accumulator);
        break;
    case 0x1:
        ROL_b(address, accumulator);
        break;
    case 0x2:
        LSR_b(address, accumulator);
        break;
    case 0x3:
        ROR_b(address, accumulator);
        break;
    case 0x4:
        STX(address);
        break;
    case 0x5:
        LDX(address);
        break;
    case 0x6:
        DECC(address);
        break;
    case 0x7:
        INC(address);
        break;
    }
    return;
}

void IRAM_ATTR run_instruction_group3(uint16_t address)
{
    uint16_t jump_address = 0;
    switch (inst.aaa)
    {
    case 0x0:
        // printf("INVALID OPCODE \n");
        break;
    case 0x1:
        BITT(address);
        break;
    case 0x2:
        jump_address = read_address_from_pc();
        JMP_abs(jump_address);
        break;
    case 0x3:
        jump_address = read_address_from_pc();
        JMP_indirect(jump_address);
        break;
    case 0x4:
        STY(address);
        break;
    case 0x5:
        LDY(address);
        break;
    case 0x6:
        CPY(address);
        break;
    case 0x7:
        CPX(address);
        break;
    }
    return;
}

void IRAM_ATTR run_instruction_group_sb1()
{
    switch (inst.opcode)
    {
    case 0x08:
        PHP();
        break;
    case 0x28:
        PLP();
        break;
    case 0x48:
        PHA();
        break;
    case 0x68:
        PLA();
        break;
    case 0x88:
        DEY();
        break;
    case 0xA8:
        TAY();
        break;
    case 0xC8:
        INY();
        break;
    case 0xE8:
        INX();
        break;
    case 0x18:
        CLC();
        break;
    case 0x38:
        SEC();
        break;
    case 0x58:
        CLI();
        break;
    case 0x78:
        SEI();
        break;
    case 0x98:
        TYA();
        break;
    case 0xB8:
        CLV();
        break;
    case 0xD8:
        CLD();
        break;
    case 0xF8:
        SED();
        break;
    default:
        break;
    }
}

void IRAM_ATTR run_instruction_group_sb2()
{
    switch (inst.opcode)
    {
    case 0x8a:
        TXA();
        break;
    case 0x9A:
        TXS();
        break;
    case 0xAA:
        TAX();
        break;
    case 0xBA:
        TSX();
        break;
    case 0xCA:
        DEX();
        break;
    case 0xEA:
        NOPP();
        break;
    default:
        break;
    }
}

void init()
{
    SP = 0xFD;
    PC = read_abs_address(0xFFFC);
    build_decode_table();
    pending_nmi = false;
}

CPU_VARS pack_vars()
{
    CPU_VARS t = {0};
    t.A = A;
    t.PC = PC - 1;
    t.X = X;
    t.Y = Y;
    t.SP = SP;
    t.cylces = cycles;
    t.flags = pack_flags();
    return t;
}

// This function runes one opcode, not one cycle. My emulator does not aim to be
// cycle accurate CPU EXECUTE NOW DOES ONLY ONE CLOCK
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

// Simplifying instruction duration calculations saves 10 ESP cycles/CPU cycle(HUGE!)
uint32_t remaining_cycles = 0;
void IRAM_ATTR cpu_clock()
{
    if (remaining_cycles)
    {
        remaining_cycles--;
    }
    else
    {
        cpu_execute();
        // fetch_instruction();
        // remaining_cycles += OPCODE_duration[inst.opcode];
    }
}

void IRAM_ATTR cpu_execute()
{
    if (!pending_nmi)
    {
        byte opcode_aux = read_pc();
        inst = decode_table[opcode_aux];
        remaining_cycles += OPCODE_duration[inst.opcode];

        byte low_nibble = low_nibble_table[inst.opcode];
        byte high_nibble = high_nibble_table[inst.opcode];
        byte last_5_bits = last_5_bits_table[inst.opcode];
        bool onaddress_group2 = false;

        uint16_t address = 0;

        if (low_nibble == 0x08)
        {
            run_instruction_group_sb1();
        }
        else if (low_nibble == 0x0A && high_nibble >= 0x08)
        {
            run_instruction_group_sb2();
        }
        else if (low_nibble == 0x00 && last_5_bits == 0b00010000)
        {
            // Branching instructions
            int8_t branch_position = (int8_t)read_pc();
            uint32_t flag = 0;
            switch (inst.xx)
            {
            case 0:
                flag = N;
                break;
            case 1:
                flag = O;
                break;
            case 2:
                flag = C;
                break;
            case 3:
                flag = Z;
                break;
            }
            if(flag == (inst.y != 0)){
                PC += branch_position;
            }
        }
        else if (inst.opcode == 0x00)
        {
            BRK();
        }
        else if (inst.opcode == 0x20)
        {
            address = read_abs_address(PC);
            PC += 1;
            JSR_abs(address);
        }
        else if (inst.opcode == 0x40)
        {
            RTI();
        }
        else if (inst.opcode == 0x60)
        {
            RTS(); // return from subroutine;
        }
        // for single byte instructions !

        else
            switch (inst.cc)
            {
            // compute_addr_mode DOES return an address via reffrence(&)
            case 0x01: // cc = 1
                address = compute_addr_mode_g1();
                // run_instruction_group1(address);
                group1_table[inst.aaa](address);
                break;
            case 0x02: // cc = 10
                // Will return address via pointer, the function returns a
                // boolean.
                onaddress_group2 = compute_addr_mode_g23(address);
                if (onaddress_group2 == true)
                    group2_table_noacc[inst.aaa](address);
                // run_instruction_group2(address,
                //                        0); // Not accumulator, on address
                else
                    group2_table_acc[inst.aaa]();
                break;
            case 0x0: // cc = 00
                compute_addr_mode_g23(address);
                run_instruction_group3(address);
                break;
            }
        return;
    }
    else
    {
        trigger_nmi();
        remaining_cycles += 7;
        return;
    }
    return; // if it gets here it means it failed
}

void inline IRAM_ATTR fetch_instruction()
{
    inst.opcode = read(PC);
    inst.aaa = (0xE0 & inst.opcode) >> 5;      // first 3 bits of the opcode
    inst.bbb = (0x1C & inst.opcode) >> 2;      // second 3 bits
    inst.cc = (0x03 & inst.opcode);            // last 2 bits
    inst.xx = (0b11000000 & inst.opcode) >> 6; // first 2 bits(xx)
    inst.y = (0b00100000 & inst.opcode) >> 5;  // third bit from the left;
}

byte IRAM_ATTR ram_at(uint16_t address)
{
    return cpu_read(address);
}
