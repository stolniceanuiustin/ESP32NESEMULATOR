#include "cartridge.h"
#include <Arduino.h>
#include <LittleFS.h>
#include "memory.h"
// Nametable_Map definition assumed to be in mapper.h or cartridge.h
Nametable_Map nametablee;


#include "cartridge.h"


Config config;
MIRROR mirroring;


void set_mapping(uint16_t top_left, uint16_t top_right, uint16_t bottom_left, uint16_t bottom_right)
{
    nametablee.map[0] = top_left;
    nametablee.map[1] = top_right;
    nametablee.map[2] = bottom_left;
    nametablee.map[3] = bottom_right;
}

bool cartridge_read_file(char* rom_name)
{
    Serial.printf("%s - ROM NAME\n", rom_name);
    File rom = LittleFS.open(rom_name, "r");
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
        int prg_size = header.prg_size * PRG_BANK_SIZE;
        int chr_size = header.chr_size * CHR_BANK_SIZE;

        if (rom.read(reinterpret_cast<byte *>(PRGrom), prg_size) != prg_size)
        {
            Serial.println("FAILED: PRG ROM read size mismatch.");
            return false;
        }
        if (prg_size == PRG_BANK_SIZE)
        {
            Serial.println("NROM-128 detected: Mirroring 16KB PRG.");
            std::memcpy(PRGrom + PRG_BANK_SIZE, PRGrom, PRG_BANK_SIZE);
        }
        if (chr_size > 0 && chr_size <= CHR_BANK_SIZE)
        {
            if (rom.read(reinterpret_cast<byte *>(CHRrom), chr_size) != chr_size)
            {
                Serial.println("FAILED: CHR ROM read size mismatch.");
                return false;
            }
        }
        else
        {
            Serial.println("CHR-Ram is present, so the mapper is not correct!");
        }
    }

    rom.close();
    return true;
}
