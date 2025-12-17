#include "cpu.h"
#include "bus.h"
#include "memory.h"
typedef uint8_t byte;

// =============BUS VARIABLES=================
byte controller[2];
byte controller_state[2];
bool dma_transfer;
bool dma_first_clock;
byte oam_dma_page = 0x00;
byte oam_dma_addr = 0x00;
byte oam_dma_data = 0x00;
uint32_t global_clock = 0;

// ==============MEMORY VARIABLES=============
byte DRAM_ATTR PRGrom[0x8000];
byte DRAM_ATTR CHRrom[0x4000];
byte DRAM_ATTR CPUram[0x0800];
byte DRAM_ATTR PPUram[0x3FFF];


// =============CPU VARIABLES=================
struct Instruction inst;
byte A = 0x00;
byte X = 0x00;
byte Y = 0x00;
uint16_t PC = 0x0000;
byte SP = 0xFD; 
volatile bool cpu_debug_print = 0;
uint32_t opcode = 0;


uint32_t cycles = 0;
int32_t estimated_cycles = 0;
uint32_t elapsed_cycles = 0;

byte C = 0; // carry
byte Z = 0; // zero
byte I = 1; // interrupt 
byte D = 0; // decimal
byte B = 0; // break
byte O = 0; // overflow
byte N = 0; // negative

bool pending_nmi = false;
bool pending_irq = false;

// ==============PPU VARIABLES============

PPUStatus DRAM_ATTR status;
PPUmask DRAM_ATTR mask;
PPUctrl DRAM_ATTR control;

State pipeline_state;
bool DRAM_ATTR pause_after_frame = false;
uint16_t DRAM_ATTR current_frame = 0;
bool DRAM_ATTR odd_frame = false;
int16_t DRAM_ATTR scanline = 0;
uint16_t DRAM_ATTR dots = 0;
bool DRAM_ATTR even_frame = false;
int DRAM_ATTR cycle = 0;

loopy DRAM_ATTR vaddr = {0x0000};
loopy DRAM_ATTR taddr = {0x0000};
uint16_t DRAM_ATTR x = 0x0000;
uint16_t DRAM_ATTR w = 0x0000;
byte DRAM_ATTR fine_x = 0;

bool DRAM_ATTR PPUSCROLL_latch = false;
bool DRAM_ATTR PPUADDR_latch = false;
bool DRAM_ATTR first_write = false;
uint16_t DRAM_ATTR PPUSCROLL16 = 0x0000;
uint16_t DRAM_ATTR PPUADDR16 = 0x0000;
byte DRAM_ATTR PPUDATA = 0x00;
byte DRAM_ATTR OAMDMA = 0x00;
byte DRAM_ATTR PPU_BUFFER = 0x00;
byte DRAM_ATTR OAMADDR = 0x00;
byte DRAM_ATTR OAMDATA = 0x00;
byte DRAM_ATTR PPUSCROLL = 0x00;

_OAM DRAM_ATTR OAM[64] = {0};
byte DRAM_ATTR *pOAM = (byte *)OAM;
byte DRAM_ATTR nametable[2][0x0400] = {0};
byte DRAM_ATTR patterntable[2][0x1000] = {0};
byte DRAM_ATTR pallete_table[32] = {0};

uint16_t DRAM_ATTR bgs_pattern_l = 0x0000;
uint16_t DRAM_ATTR bgs_pattern_h = 0x0000;
uint16_t DRAM_ATTR bgs_attribute_l = 0x0000;
uint16_t DRAM_ATTR bgs_attribute_h = 0x0000;
byte DRAM_ATTR bg_next_tile_id = 0x00;
byte DRAM_ATTR bg_next_tile_attrib = 0x00;
byte DRAM_ATTR bg_next_tile_lsb = 0x00;
byte DRAM_ATTR bg_next_tile_msb = 0x00;

_OAM DRAM_ATTR sprites_on_scanline[8] = {0};
byte DRAM_ATTR sprite_cnt = 0;
byte DRAM_ATTR sp_pattern_l[8] = {0};
byte DRAM_ATTR sp_pattern_h[8] = {0};
bool DRAM_ATTR sprite_zero_on_scanline = false;
bool DRAM_ATTR sprite_zero_is_rendering = false;

byte DRAM_ATTR flip_byte[256] = {0};
byte DRAM_ATTR tile_fetch_phase = 0;

byte transparent_pixel_color = 0;
byte scanline_buffer[256] = {0};
byte tile_pixels[512][8][8] = {0};
bool tile_cache_initialized = false;