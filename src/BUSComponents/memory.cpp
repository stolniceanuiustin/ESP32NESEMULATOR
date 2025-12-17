#include "memory.h"
// From my understanding, there macros should help the branch predictor.
// But from my profiling it doesn't seem to be that big of a deal. 
#define LIKELY(x)   (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))



//*** This file has the implementation for ALL the memory related functions.*** 

// TODO : When Emulator reaches 60 fps, implement Mappers

byte cpu_read(uint16_t addr)
{
    if (LIKELY(addr >= 0x8000))
    {
        return PRGrom[addr & 0x7FFF];
    }
    else if (LIKELY(addr <= 0x1FFF))
    {
        return CPUram[addr & 0x07FF];
    }
    else if (UNLIKELY(addr >= 0x2000 && addr <= 0x3FFF))
    {
        // Let's assume for now that the CPU only addresses the registers via their real address and not via mirroring
        byte data = ppu_read_from_cpu(addr /*& 0x0007*/);
        return data;
    }
    else if (addr >= 0x4000 && addr <= 0x4015)
    {
        return 0;
    }
    else if (addr == 0x4016 || addr == 0x4017)
    {
        byte data = (controller_state[addr & 1] & 0x80) > 0 ? 1 : 0;
        controller_state[addr & 1] <<= 1;
        return data; // bit 6 shjould always be set to 1 due to open bus behaviour
    }
    else
        return 0;
}

void cpu_write(uint16_t addr, byte data)
{
    if (addr >= 0x8000 && addr <= 0xFFFF)
    {
        PRGrom[addr & (0x7FFF)] = data;
    }
    else if (addr >= 0x0000 && addr <= 0x1FFF)
    {
        CPUram[addr& 0x07FF] = data;
    }
    else if (addr >= 0x2000 && addr <= 0x3FFF) // those are the PPU registers mirrored every 8 bites
    {
        ppu_write_from_cpu(addr /*& 0x0007*/, data);    // We asume the CPU only writes to 2000, 2001, 2002..2007 and not any of the mirrored addresses
    }
    else if (addr >= 0x4000 && addr <= 0x4013)
    {
        // std::cerr << "APU REGISTERS\n";
    }
    else if (addr == 0x4014) // OAMDMA
    {
        // This is no longer cycle accurate but we make that tradeoff for better performance
        uint16_t dma_start_addr = (uint16_t)data << 8;
        memcpy(pOAM, &CPUram[dma_start_addr], 256);
        extern uint32_t remaining_cycles;
        remaining_cycles += 513;
    }
    else if (addr == 0x4016 || addr == 0x4017)
    {
        static byte prev_write = 0;
        if ((prev_write & 1) == 1 && (data & 1) == 0)
        {
            controller_state[0] = controller[0];
            controller_state[1] = 0x00;
        }
        prev_write = data;
    }
}