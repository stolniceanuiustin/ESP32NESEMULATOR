#ifndef BUS_HEADERGUARD
#define BUS_HEADERGUARD
#include <stdint.h>

typedef uint8_t byte;

extern uint32_t new_cycles;
extern int32_t old_cycles;
extern uint32_t cycles_elapsed;

extern byte controller[2];
extern byte controller_state[2];
extern bool dma_transfer;
extern bool dma_first_clock;
extern byte oam_dma_page;
extern byte oam_dma_addr;
extern byte oam_dma_data;
void bus_init();

extern uint32_t global_clock;
void bus_reset();
void bus_clock();

#endif