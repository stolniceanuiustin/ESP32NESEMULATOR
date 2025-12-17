#include "cpu.h"

// This contains all the address logic, as explained here:
// https://www.nesdev.org/obelisk-6502-guide/addressing.html

const byte last_byte = 0xFF; // 00...011111111
uint16_t addr_IMM()
{
    uint16_t addr = PC++;
    return addr;
}

// this takes (zero_page + x) as pointer
uint16_t addr_zero_page_x_p()
{
    byte zeropage_adr;
    zeropage_adr = read_pc() + X;
    byte low_byte = read(zeropage_adr);
    // IT DOES NOT WRAP ARROUND BY ITS OWN FOR SOME REASON;
    byte high_byte = read(((zeropage_adr + 1) & 0x00FF));
    return (high_byte << 8) | low_byte;
}
// zero_page + x as absolute address
uint16_t addr_zero_page_x()
{
    uint16_t address;
    address = read_pc() + X;
    address &= last_byte;
    return address;
}
// zero_page + y as absolute address
uint16_t addr_zero_page_y()
{
    uint16_t address;
    address = read_pc() + Y;
    //only take the last_byte. 0x0F = last_byte
    address &= last_byte;
    return address;
}
//just an absolute address that is from zero page(as its only one byte wide)
uint16_t addr_zero_page()
{
    return read_pc();
}
uint16_t addr_abs()
{
    uint16_t address = read_abs_address(PC);
    PC += 2;
    return address;
}
//(zero page), Y; Takes an address from zero page as a pointer then adds Y to that address
uint16_t addr_pzero_page_y()
{
    uint16_t address;
    uint16_t zeropage_adr = read_pc();
    byte low_byte = read(zeropage_adr);
    byte high_byte = read((zeropage_adr + 1) & 0x00FF);
    address = (high_byte << 8) | low_byte;
    uint16_t aux_addr = address;
    address += Y;
    return address;
}

uint16_t addr_abs_y()
{
    uint16_t address = read_abs_address(PC);
    address += Y;
    PC += 2;
    return address;
}

uint16_t addr_abs_x()
{
    uint16_t address = read_abs_address(PC);
    PC += 2;
    address += X;
    return address;
}

