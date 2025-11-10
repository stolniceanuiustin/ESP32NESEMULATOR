#include "cartridge.h"
#include "mapper.h"
#include <Arduino.h>
#include <LittleFS.h>
// Nametable_Map definition assumed to be in mapper.h or cartridge.h
Nametable_Map nametablee;

void set_mapping(uint16_t top_left, uint16_t top_right, uint16_t bottom_left, uint16_t bottom_right)
{
    nametablee.map[0] = top_left;
    nametablee.map[1] = top_right;
    nametablee.map[2] = bottom_left;
    nametablee.map[3] = bottom_right;
}

bool CARTRIDGE::read_file()
{
    File rom = LittleFS.open(config.rom_name, "r");
    if (!rom)
    {
        Serial.println("Could not open ROM FILE");
        return false;
    }

    NESHeader header;
    // Read the 16-byte iNES header
    rom.read(reinterpret_cast<byte *>(&header), sizeof(header));

    if (header.magic[0] != 'N' || header.magic[1] != 'E' || header.magic[2] != 'S' || header.magic[3] != 0x1A)
    {
        Serial.println("Invalid NES file: Missing iNES header.");
        rom.close();
        return false;
    }

    
    Serial.print((int)header.flags6);
    Serial.print(" ");
    Serial.println((int)header.flags7); 
    
    // Skip Trainer if present (Flags 6 bit 2 is set)
    if (header.flags6 & 0x04) 
    {
        Serial.println("Skipping 512-byte Trainer.");
        rom.seek(512, SeekCur);
    }
    
    byte mapper_type = ((header.flags6 & 0b11110000) >> 4) | (header.flags7 & 0b11110000);
    Serial.print("Mapper_type: ");
    Serial.println((int)mapper_type);
    
    byte nametable_type = (header.flags6 & 1);
    if (nametable_type == 0)
    {
      
        Serial.println("Using vertical mirroring");
        mirroring = VERTICAL;
        set_mapping(0, 0x400, 0, 0x400);
    }
    else // nametable_type == 1
    {
        mirroring = HORIZONTAL;
        Serial.println("Using horizontal mirroring");
        set_mapping(0, 0, 0x400, 0x400);
    }
    config.nametable_arrangement = (nametable_type == 1) ? 1 : 0;

    if (mapper_type == 0)
    {
        Serial.println("Using mappertype 0");
        p_mapper = std::make_shared<Mapper0>(header.prg_size, header.chr_size);
        mapper0(config, *this, rom);
    }
    else if (mapper_type == 1)
    {
        Serial.println("Using mappertype 1");
        p_mapper = std::make_shared<Mapper1>(header.prg_size, header.chr_size);
        mapper1(config, *this, rom);
    }
    
    rom.close();
    return true;
}

byte CARTRIDGE::cpu_read(uint16_t addr)
{
    uint16_t mapped_addr = p_mapper->cpu_map_read(addr);
    return PRGrom[mapped_addr];
}
void CARTRIDGE::cpu_write(uint16_t addr, byte data)
{
    uint16_t mapped_addr = p_mapper->cpu_map_write(addr);
    PRGrom[mapped_addr] = data;
}
byte CARTRIDGE::ppu_read(uint16_t addr)
{
    uint16_t mapped_addr = p_mapper->ppu_map_read(addr);
    return CHRrom[mapped_addr];
}
void CARTRIDGE::ppu_write(uint16_t addr, byte data)
{
    uint16_t mapped_addr = p_mapper->ppu_map_write(addr);
    CHRrom[mapped_addr] = data;
}