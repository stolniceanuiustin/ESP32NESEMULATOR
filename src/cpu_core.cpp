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

const uint16_t null_address = 0;


void cpu_init() {
	SP = 0xFD;
	PC = read_abs_address(RESET_VECTOR);
	pending_nmi = false;
}
void cpu_reset() {
	A = 0;
	X = 0;
	Y = 0;
	SP = 0xFD;	// it decreases with pushing
	PC = read_abs_address(RESET_VECTOR);
	C = 0;	// carry
	Z = 0;	// zero
	I = 1;
	D = 0;
	B = 0;
	O = 0;
	N = 0;
	bus_reset();
	cycles = 0;
	elapsed_cycles = 0;
	estimated_cycles = 0;
	state = FETCH_INSTRUCTION;
}

byte IRAM_ATTR read_pc()
{
    byte val = read(PC);
    PC++;
    return val;
}

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
uint16_t IRAM_ATTR  read_abs_address(uint16_t offset)
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

bool reset()
{
    A = 0;
    X = 0;
    Y = 0;
    SP = 0xFD; // it decreases with pushing
    PC = read_abs_address(RESET_VECTOR);
    C = 0; // carry
    Z = 0; // zero
    I = 1;
    D = 0;
    B = 0;
    O = 0;
    N = 0;
    bus_reset();
    cycles = 0;
    elapsed_cycles = 0;
    estimated_cycles = 0;
    state = FETCH_INSTRUCTION;
    return true;
}

void IRAM_ATTR run_instruction_group1(uint16_t address, bool page_cross)
{
    switch (inst.aaa)
    {
    case 0x0:
        ORA(address, page_cross);
        break;
    case 0x1:
        AND(address, page_cross);
        break;
    case 0x2:
        EOR(address, page_cross);
        break;
    case 0x3:
        ADC(address, page_cross);
        break;
    case 0x4:
        STA(address);
        break;
    case 0x5:
        LDA(address, page_cross);
        break;
    case 0x6:
        CMP(address, page_cross);
        break;
    case 0x7:
        SBC(address, page_cross);
        break;
    }
}

void IRAM_ATTR run_instruction_group2(uint16_t address, bool page_cross, bool accumulator)
{
    switch (inst.aaa)
    {
    case 0x0:
        ASL(address, accumulator);
        break;
    case 0x1:
        ROL(address, accumulator);
        break;
    case 0x2:
        LSR(address, accumulator);
        break;
    case 0x3:
        ROR(address, accumulator);
        break;
    case 0x4:
        STX(address);
        break;
    case 0x5:
        LDX(address, page_cross);
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

void IRAM_ATTR run_instruction_group3(uint16_t address, bool page_cross)
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
        LDY(address, page_cross);
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

void IRAM_ATTR  run_instruction_group_sb2()
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

void IRAM_ATTR cpu_clock()
{
    if (state == FETCH_INSTRUCTION)
    {
        fetch_instruction();
        estimated_cycles = estimate_cycles();
        if (estimated_cycles == -1)
        {
            estimated_cycles = 2;
        }
        elapsed_cycles = 1;
        state = WAIT_FOR_N_CYCLES;
    }
    else if (state == WAIT_FOR_N_CYCLES)
    {
        elapsed_cycles++;
        if (elapsed_cycles == estimated_cycles)
        {
            state = EXECUTE;
        }
    }
    if (state == EXECUTE)
    {
        state = FETCH_INSTRUCTION;
        cpu_execute();
    }
}


// TODO: CHange this to be a boolean.
void IRAM_ATTR cpu_execute()
{
    CPU_VARS cpu_old_vars;
    if (pending_nmi == true)
    {
        trigger_nmi();
        state = WAIT_FOR_N_CYCLES;
        estimated_cycles = 7;
        elapsed_cycles = 1;
        return;
    }
    else
    {
        inst.opcode = read_pc();
        inst.aaa = (0xE0 & inst.opcode) >> 5;      // first 3 bits of the opcode
        inst.bbb = (0x1C & inst.opcode) >> 2;      // second 3 bits
        inst.cc = (0x03 & inst.opcode);            // last 2 bits
        inst.xx = (0b11000000 & inst.opcode) >> 6; // first 2 bits(xx)
        inst.y = (0b00100000 & inst.opcode) >> 5;  // third bit from the left;
        bool onaddress_group2 = false;
        bool page_cross = false;
        if (PC == 0xFFFF)
        {
            return;
        }
        byte last_5_bits = (0b00011111 & inst.opcode);
        byte low_nibble = inst.opcode & 0x0F;
        byte high_nibble = inst.opcode >> 4;
        uint16_t address = 0;

        // Some unofficial opcodes(not all yet)
        if (inst.opcode == 0x04 || inst.opcode == 0x44 || inst.opcode == 0x64)
        {
            PC += 1;
            cycles += 3;
        }
        else if (inst.opcode == 0x0C)
        {
            PC += 2;
            cycles += 4;
        }
        else if (inst.opcode == 0x14 || inst.opcode == 0x34 || inst.opcode == 0x54 ||
                 inst.opcode == 0x74 || inst.opcode == 0xD4 || inst.opcode == 0xF4)
        {
            PC += 1;
            cycles += 4;
        }

        else if (low_nibble == 0x00 && last_5_bits == 0b00010000)
        {
            // Branching instructions
            int8_t branch_position = (int8_t)read_pc();
            bool branch_succeded = false;
            bool page_cross = false;
            uint16_t aux_pc = PC;
            if (inst.xx == 0b00 && N == inst.y)
                branch_succeded = true;
            else if (inst.xx == 0b01 && O == inst.y)
                branch_succeded = true;
            else if (inst.xx == 0b10 && C == inst.y)
                branch_succeded = true;
            else if (inst.xx == 0b11 && Z == inst.y)
                branch_succeded = true;
            if (branch_succeded)
            {
                PC += branch_position;
                page_cross = (aux_pc & 0xFF00) != (PC & 0xFF00);
            }
            cycles += 2 + (int)branch_succeded + 2 * (int)page_cross;
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
        else if (low_nibble == 0x08)
        {
            run_instruction_group_sb1();
        }
        else if (low_nibble == 0x0A && high_nibble >= 0x08)
        {
            run_instruction_group_sb2();
        }
        else
            switch (inst.cc)
            {
            // compute_addr_mode DOES return an address via reffrence(&)
            case 0x01: // cc = 1
                address = compute_addr_mode_g1(page_cross);
                run_instruction_group1(address, page_cross);
                break;
            case 0x02: // cc = 10
                // Will return address via pointer, the function returns a
                // boolean.
                onaddress_group2 = compute_addr_mode_g23(page_cross, address);
                if (onaddress_group2 == true)
                    run_instruction_group2(address, page_cross,
                                           0); // Not accumulator, on address
                else
                    run_instruction_group2(null_address, page_cross,
                                           1); // On accumulator
                break;
            case 0x0: // cc = 00
                compute_addr_mode_g23(page_cross, address);
                run_instruction_group3(address, page_cross);
                break;
            }
        return;
    }
    return; //if it gets here it means it failed
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