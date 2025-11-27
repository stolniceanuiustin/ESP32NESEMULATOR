#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <cstring>
#include <iostream>

#include "bus.h"
#include "cpu.h"
#include "ppu.h"
#include "cpu_vars.h"

void cpu_init()
{
    SP = 0xFD;
    PC = read_abs_address(RESET_VECTOR);
    pending_nmi = false;
    init_instruction_handler_lut();
    build_decode_table();
}
void cpu_reset()
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
    init_instruction_handler_lut();
    build_decode_table();
}

