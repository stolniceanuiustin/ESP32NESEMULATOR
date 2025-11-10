#include <stdint.h>
#include <Arduino.h>
#include <cstring> 
#include "emulator_config.h"
#include "memory.h"
#include "mapper.h"
#include "cartridge.h"
#include <LittleFS.h>

bool mapper0(Config &config, CARTRIDGE& cartridge, File &rom)
{
    const int PRG_BANK_SIZE = 16 * 1024;
    const int CHR_BANK_SIZE = 8 * 1024;

    int prg_size = cartridge.get_prg_size() * PRG_BANK_SIZE;
    int chr_size = cartridge.get_chr_size() * CHR_BANK_SIZE;
    
    if (rom.read(reinterpret_cast<byte *>(cartridge.get_PRGrom()), prg_size) != prg_size)
    {
        Serial.println("FAILED: PRG ROM read size mismatch.");
        return false;
    }

   
    if (prg_size == PRG_BANK_SIZE) 
    {
        
        Serial.println("NROM-128 detected: Mirroring 16KB PRG.");
        std::memcpy(cartridge.get_PRGrom() + PRG_BANK_SIZE, cartridge.get_PRGrom(), PRG_BANK_SIZE);
    }

    if (chr_size > 0 && chr_size <= CHR_BANK_SIZE) 
    {
        if (rom.read(reinterpret_cast<byte *>(cartridge.get_CHRrom()), chr_size) != chr_size)
        {
            Serial.println("FAILED: CHR ROM read size mismatch.");
            return false;
        }
    }
    else
    {
        Serial.println("CHR-Ram is present, so the mapper is not correct!");
    }
    return true;
}



uint16_t Mapper0::cpu_map_read(uint16_t addr)
{
    if(addr >= 0x8000 && addr <= 0xFFFF)
    {
        return addr & (0x7FFF); 
    }
    else return 0; 
}
uint16_t Mapper0::cpu_map_write(uint16_t addr)
{
    if(addr >= 0x8000 && addr <= 0xFFFF)
    {
        return addr & 0x7FFF;
    }
    else return 0;
}
uint16_t Mapper0::ppu_map_read(uint16_t addr)
{
    return addr;
}
//Only for games with CHRram, will have to implement later; TODO
uint16_t Mapper0::ppu_map_write(uint16_t addr) 
{
    return 0;
}