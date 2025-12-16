#include <iostream>
#include <unistd.h>
#include "emulator_config.h"
#include "cpu.h"
#include "ppu.h"
#include "bus.h"
#include "cartridge.h"

void bus_init()
{
    dma_transfer = false;
    dma_first_clock = true;
}

void bus_reset()
{
    controller[0] = 0x00;
    controller[1] = 0x00;
    controller_state[0] = 0x00;
    controller_state[1] = 0x00;
    dma_transfer = false;
    oam_dma_page = 0x00;
    oam_dma_addr = 0x00;
    global_clock = 1;
}

// void BUS::hexdump()
// {
//     cpu_ram.hexdump("cpu_hexdump", 0x07FF);
//     ppu_ram.hexdump("ppu_hexdump", 0x3FFF);
// }

// This is the old function that clocked the whole system. Its a legacy function ported from my x86 emulator but it should not be used here
void bus_clock()
{
    ppu_execute();
    if (global_clock % 3 == 0)
    {
        if (dma_transfer == true)
        {
            if (dma_first_clock)
            {
                if ((global_clock & 1) == 0) // instead of modulo
                {
                    dma_first_clock = false;
                }
            }
            else
            {
                if (global_clock & 1) // instead of $2 == 0
                {
                    oam_dma_data = cpu_read(oam_dma_page << 8 | oam_dma_addr);
                }
                else
                {
                    pOAM[oam_dma_addr] = oam_dma_data;
                    oam_dma_addr++;
                    if (oam_dma_addr == 0x00)
                    {
                        dma_transfer = false;
                        dma_first_clock = true;
                    }
                }
            }
        }
        else
        {
            //cpu_clock();
        }
    }
    global_clock++;
}