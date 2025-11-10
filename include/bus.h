#ifndef BUS_HEADERGUARD
#define BUS_HEADERGUARD
#include <stdint.h>
#include "memory.h"
typedef uint8_t byte;
class CPU;
class PPU;
class Memory;
class CARTRIDGE;

class BUS
{
public:
    uint64_t new_cycles = 0;
    uint64_t old_cycles = 0;
    uint64_t cycles_elapsed = 0;
    CPU &cpu;
    PPU &ppu;
    CARTRIDGE& cartridge;
    Memory cpu_ram;
    Memory ppu_ram;
    byte controller[2];
    byte controller_state[2];
    bool dma_transfer;
    bool dma_first_clock;
    byte oam_dma_page = 0x00;
    byte oam_dma_addr = 0x00;
    byte oam_dma_data = 0x00;
    BUS(CPU &cpu, PPU &ppu, CARTRIDGE& cartridge) : cpu(cpu), ppu(ppu), cartridge(cartridge), cpu_ram(0x0800), ppu_ram(0x3FFF) {
        dma_transfer = false;
        dma_first_clock = true;
    };
    
    uint64_t global_clock = 0;
    void cpu_write(uint16_t addr, byte data);
    void ppu_write(uint16_t addr, byte data);
    byte cpu_read(uint16_t addr);
    byte ppu_read(uint16_t addr);
    void reset();
    void hexdump();
    void clock(bool debug);
    void clock();
};

#endif