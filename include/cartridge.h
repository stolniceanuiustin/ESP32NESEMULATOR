#ifndef CARTRIDGE_HEADERGUARD
#define CARTRIDGE_HEADERGUARD
#include <Arduino.h>
#include <stdint.h>
#include <memory>
#include "emulator_config.h"
#include "memory.h"
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
extern Config config;


extern MIRROR mirroring;

bool cartridge_read_file(char* rom_name);
const int PRG_BANK_SIZE = 16 * 1024;
const int CHR_BANK_SIZE = 8 * 1024;




#endif