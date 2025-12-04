#ifndef HEADERGUARD_PPU
#define HEADERGUARD_PPU
#include <stdint.h>
#include "virtual_screen.h"
#include "cartridge.h"
#include "memory.h"

#define VISIBLE_SCANLINES 240
#define VISIBLE_DOTS 256
#define FRAME_END_SCANLINE 261
#define DOTS_PER_SCANLINE 341
#define LAST_SCANLINE_DOT 340

typedef uint8_t byte;

// this is size 4
struct _OAM
{
    byte y;
    byte id;
    byte attributes;
    byte x;
}; // this is how a sprite is stored

extern union PPUStatus
{
    //Changed ORDER because endiness on ESP32 is backwards
    struct
    {
        uint8_t unused : 5;          
        uint8_t sprite_overflow : 1; 
        uint8_t sprite_zero_hit : 1; 
        uint8_t vertical_blank : 1;  
    };
    byte reg;
} status;

extern union PPUmask
{
    struct
    {
        byte grayscale : 1;
        byte render_background_left : 1;
        byte render_sprites_left : 1;
        byte render_background : 1;
        byte render_sprites : 1;
        byte enhance_red : 1;
        byte enhance_green : 1;
        byte enhance_blue : 1;
    };

    byte reg;
} mask;

extern union PPUctrl
{
    struct
    {
        byte nametable_x : 1;
        byte nametable_y : 1;
        byte increment_mode : 1;
        byte pattern_sprite : 1;
        byte pattern_background : 1;
        byte sprite_size : 1;
        byte slave_mode : 1; // UNUSED
        byte enable_nmi : 1;
    };
    byte reg;
} control;

union loopy
{
    // THE X and T registers play more roles. Can either be used as address for vram or for selecting other stuff
    struct
    {
        uint16_t coarse_x : 5;
        uint16_t coarse_y : 5;
        uint16_t nametable_x : 1;
        uint16_t nametable_y : 1;
        uint16_t fine_y : 3;
        uint16_t unused : 1;
    };
    uint16_t reg;
};

extern bool pause_after_frame;
extern uint16_t current_frame;
extern bool odd_frame;

// byte bus;
extern loopy vaddr; 
// in render = scrolling position, outside of rendering = the current VRAM address
extern loopy taddr; 
// During rendering it does something. Outsdie of rendering, holds the VRAM address before transfering it to v
extern uint16_t x; 
// FINE x position of the current scroll, sused during rendering alongside v
extern uint16_t w; 
// Togles on each write to PPUSCROLL or PPUADDR, indicating whether it's the first or secnon dwrite. Clears on reads of PPUSTATUS
// its also claled the write latch or write toggle

extern int16_t scanline;
extern uint16_t dots;
extern byte transparent_pixel_color;
extern bool PPUSCROLL_latch;
extern bool PPUADDR_latch;
extern bool first_write;

extern uint16_t PPUSCROLL16;
extern uint16_t PPUADDR16;
extern byte PPUDATA;
extern byte OAMDMA;
extern byte PPU_BUFFER;
extern bool even_frame;
extern _OAM OAM[64];
extern byte *pOAM; // pointer to OAM for byte by byte access

extern byte tile_fetch_phase;
// TODO: here you can enable/disable ppu logging
void ppu_init();


extern byte OAMADDR;
extern byte OAMDATA;
extern byte PPUSCROLL;

extern byte nametable[2][0x0400]; // mirrored
extern byte patterntable[2][0x1000];
extern byte pallete_table[32];
extern byte fine_x; // 3 bits wide! ONLY 3 BIT WIDE
// rendering shift registers! they shift every PPU clock. There are 2 16bit registers
// https://www.nesdev.org/wiki/PPU_rendering
// Conceptually:
// Pixels that are being drawn currently
// So we always load only the last 8 bits of the register
// msb xxxx-xxxx                            xxxx-xxxx
// lsb xxxx-xxxx                            xxxx-xxxx
// BACKGROUND RENDEERING
extern uint16_t bgs_pattern_l;
extern uint16_t bgs_pattern_h;
extern uint16_t bgs_attribute_l;
extern uint16_t bgs_attribute_h;
extern byte bg_next_tile_id;
extern byte bg_next_tile_attrib;
extern byte bg_next_tile_lsb;
extern byte bg_next_tile_msb;

// FOREGROUND RENDERING
extern _OAM sprites_on_scanline[8]; // only 8 sprites per scanline
extern byte sprite_cnt;
extern byte sp_pattern_l[8];
extern byte sp_pattern_h[8];
extern bool sprite_zero_on_scanline;
extern bool sprite_zero_is_rendering;
void ppu_reset();
void ppu_render_scanline();
void ppu_execute();
void set_vblank_nmi();
void clear_vblank_nmi();
void set_vblank();
void clear_vblank();
byte get_status();
byte get_control();
// For main bus

byte ppu_read_from_cpu(byte addr);
void ppu_write_from_cpu(byte addr, byte data);
// for internal ppu bus

byte ppu_read(uint16_t addr);
void ppu_write(uint16_t addr, byte data);
// LOG ppu_log;
void hexdump();

// PPU Helper functions and variables
extern byte flip_byte[256];
void generate_flip_byte_lt();
void build_tile_cache();


void clock_shifters();

inline void load_bg_shifters();
inline void transfer_address_x();
inline void increment_scroll_x();
inline void increment_scroll_y();
inline void transfer_address_y();

// void memset_loop_unrolled(); //This is specifically for PPU
void init_sprites_on_scanline();
loopy build_background_scanline(int scanline_index, loopy vaddr_snapshot, byte fine_x_snapshot);
enum State
{
    PRE_RENDER,
    RENDER,
    POST_RENDER,
    VERTICAL_BLANK
};
extern State pipeline_state;
extern int cycle;


//Optimizations 
extern byte scanline_buffer[256];
extern byte tile_pixels[512][8][8]; //Decode CHR tiles in advance - for Mapper0 games this never changes
extern bool tile_cache_initialized;
byte ppu_read_nametable_tile(byte nt_x, byte nt_y, byte coarse_x, byte coarse_y);
#endif