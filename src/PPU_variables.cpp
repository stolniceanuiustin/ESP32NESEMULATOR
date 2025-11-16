#include "ppu.h"

PPUStatus status;

PPUmask mask;

PPUctrl control;
State pipeline_state;

bool pause_after_frame = false;
uint16_t current_frame = 0;
bool odd_frame = false;
int scanline = 0;
uint16_t dots = 0;
bool even_frame = false;
int cycle = 0;

loopy vaddr = { 0x0000 };
loopy taddr = { 0x0000 };
uint16_t x = 0x0000;
uint16_t w = 0x0000;
byte fine_x = 0;

bool PPUSCROLL_latch = false;
bool PPUADDR_latch = false;
bool first_write = false;
uint16_t PPUSCROLL16 = 0x0000;
uint16_t PPUADDR16 = 0x0000;
byte PPUDATA = 0x00;
byte OAMDMA = 0x00;
byte PPU_BUFFER = 0x00;
byte OAMADDR = 0x00;
byte OAMDATA = 0x00;
byte PPUSCROLL = 0x00;

_OAM OAM[64] = {0};
byte *pOAM = (byte *)OAM;
byte nametable[2][0x0400] = {0};
byte patterntable[2][0x1000] = {0};
byte pallete_table[32] = {0};

uint16_t bgs_pattern_l = 0x0000;
uint16_t bgs_pattern_h = 0x0000;
uint16_t bgs_attribute_l = 0x0000;
uint16_t bgs_attribute_h = 0x0000;
byte bg_next_tile_id = 0x00;
byte bg_next_tile_attrib = 0x00;
byte bg_next_tile_lsb = 0x00;
byte bg_next_tile_msb = 0x00;

_OAM sprites_on_scanline[8] = {0};
byte sprite_cnt = 0;
byte sp_pattern_l[8] = {0};
byte sp_pattern_h[8] = {0};
bool sprite_zero_on_scanline = false;
bool sprite_zero_is_rendering = false;

byte flip_byte[256] = {0};