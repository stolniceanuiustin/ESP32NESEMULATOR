#include "bus.h"

typedef uint8_t byte;

byte controller[2];
byte controller_state[2];
bool dma_transfer;
bool dma_first_clock;
byte oam_dma_page = 0x00;
byte oam_dma_addr = 0x00;
byte oam_dma_data = 0x00;
uint32_t global_clock = 0;
