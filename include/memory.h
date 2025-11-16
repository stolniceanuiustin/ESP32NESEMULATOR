#ifndef MEMORY_HEADERGUARD
#define MEMORY_HEADERGUARD
#include "ppu.h"
#include "bus.h"
#include <Arduino.h>
#include <stdint.h>
#include <cstring>
#include <stdio.h>


typedef uint8_t byte;



extern byte PRGrom[0x8000];
extern byte CHRrom[0x4000];
extern byte CPUram[0x0800];
extern byte PPUram[0x3FFF];

byte cpu_read(uint16_t addr);
void cpu_write(uint16_t addr, byte data);

#endif
