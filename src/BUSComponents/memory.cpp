#include "memory.h"

// From my understanding, there macros should help the branch predictor.
// But from my profiling it doesn't seem to be make any difference. From what i read this is dependant on the platform you run the code on.
#define LIKELY(x) (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))

// This file has the implementation for ALL the memory related functions.

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
        byte data = ppu_read_from_cpu(addr & 0x0007);
        return data;
    }
    else if (UNLIKELY(addr >= 0x4000 && addr <= 0x4015))
    {
        // Those are Audio Registers. Not yet implemented!
        return 0;
    }
    else if (UNLIKELY(addr == 0x4016 || addr == 0x4017))
    {
        // This is, in reality, a shift register. Every time the cpu reads this, it only gets one byte
        // The cpu might read 8 times in a row, and you get the buttons in this order:
        // A, B, Select, Start, Up, Down, Left, Right
        // Fun Fact: they made it that way so the controller could only have 5 pins - VCC, GND, Latch, Clock and Data
        byte data = (controller_state[addr & 1] & 0x80) > 0 ? 1 : 0;
        controller_state[addr & 1] <<= 1;
        return data;
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
        CPUram[addr & 0x07FF] = data;
    }
    else if (addr >= 0x2000 && addr <= 0x3FFF)
    {
        ppu_write_from_cpu(addr & 0x0007, data);
    }
    else if (addr >= 0x4000 && addr <= 0x4013)
    {
        // Those are
    }
    else if (addr == 0x4014) // OAMDMA
    {
        // Writing in this register triggers a DMA between ram and the PPU object attribute memory
        // A DMA here is really just a Blocking Burst Transfer, as
        // This is no longer cycle accurate but we make that tradeoff for better performance
        // Worst case, we drift one cycle away.

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

void memory_init()
{
    // extern byte PRGrom[0x8000];
    // extern byte CHRrom[0x4000];
    // extern byte CPUram[0x0800];
    // extern byte PPUram[0x3FFF];
    memset(PRGrom, 0, 0x8000);
    memset(CHRrom, 0, 0x4000);
    memset(CPUram, 0, 0x0800);
    memset(PPUram, 0, 0x3FFF);
    
}