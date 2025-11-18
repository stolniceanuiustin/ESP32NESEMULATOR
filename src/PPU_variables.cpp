#include "ppu.h"

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