#include "memory.h"


byte cpu_read(uint16_t addr)
{
    if (addr >= 0x8000 && addr <= 0xFFFF)
    {
        // uint16_t mapped_addr = p_mapper->cpu_map_read(addr);
        // return PRGrom[mapped_addr];
        //hardcoding mapper0. 
        return PRGrom[addr & (0x7FFF)]; 
    }
    if (addr >= 0x0000 && addr <= 0x1FFF)
    {
        return CPUram[addr & 0x07FF];
    }
    else if (addr >= 0x2000 && addr <= 0x3FFF)
    {
        byte data = ppu_read_from_cpu(addr & 0x0007, false);
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
        CPUram[addr & 0x07FF] = data;
    }
    else if (addr >= 0x2000 && addr <= 0x3FFF) // those are the PPU registers mirrored every 8 bites
    {
        ppu_write_from_cpu(addr & 0x0007, data);
    }
    else if (addr >= 0x4000 && addr <= 0x4013)
    {
        // std::cerr << "APU REGISTERS\n";
    }
    else if (addr == 0x4014) // OAMDMA
    {
        oam_dma_page = data;
        oam_dma_addr = 0x00;
        dma_transfer = true;
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