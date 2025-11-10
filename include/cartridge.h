#ifndef CARTRIDGE_HEADERGUARD
#define CARTRIDGE_HEADERGUARD
#include <Arduino.h>
#include <stdint.h>
#include <memory>
#include "emulator_config.h"
#include "memory.h"
#include "mapper.h"
typedef uint8_t byte;

struct Nametable_Map
{
    uint16_t map[4];
};

enum MIRROR
{
    VERTICAL,
    HORIZONTAL
};

class CARTRIDGE
{
private:
    Config config;
    Memory PRGrom;
    Memory CHRrom;
    std::shared_ptr<Mapper> p_mapper;
public: 
    MIRROR mirroring;
    CARTRIDGE(Config config) : config(config), PRGrom(0x8000), CHRrom(0x4000)
    {};
    bool read_file();
    byte* get_PRGrom()
    {
        return &PRGrom[0x0000];
    }
    byte* get_CHRrom()
    {
        return &CHRrom[0x0000];
    }
    byte get_chr_size()
    {
        return p_mapper->chr_banks;
    }
    byte get_prg_size()
    {
        return p_mapper->prg_banks;
    }
    
    byte cpu_read(uint16_t addr);
    void cpu_write(uint16_t addr, byte data);
    byte ppu_read(uint16_t addr);
    void ppu_write(uint16_t addr, byte data);
};



#endif