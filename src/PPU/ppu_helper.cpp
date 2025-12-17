#include "ppu.h"
#include <Arduino.h>

// Each 8x8 backgroud tile is encoded by 16 bytes
// For each pixel - 2 bit color value that index in the pallete
// Normally, for CHRram, we should update this, but since we focus on MAPPER0 for now we don't do that
// Mapper0 doesn't have CHRram
// We call this once per scnaline
void build_tile_cache()
{
    //there are 512 tiles kept in memory
    for(int i=0; i<512; i++)
    {
        uint16_t base = i * 16;
        for(int row = 0; row < 8; row++)
        {
            //respect the endiness
            byte low = CHRrom[base+row];
            byte hi = CHRrom[base+row+8];
            for(int col = 0; col <8; col++){
                byte bit = ((hi >> (7 - col)) & 1) << 1 | ((low >> (7 - col)) & 1);
                tile_pixels[i][row][col] = bit;
            }
        }
    }
    tile_cache_initialized = true;
}


// This function returns which tile is used at a given tile position in a given nametable
// Pattern tables are stored at 0x0000 - they are 8x8 pixels, stored as bitplanes, which represent the raw shapes 
// The PPU has 4 nametables, starting at address 0x2000. They are each 32x30 tiles, one byte each -> an index into the pattern table 
// The missing 32x2 represents the pallete table
// Conceptually, this is how a background tile is rendered:
// 1. The tile index is read from a nametable -> You get the hsape
// 2. Use tile index to fetch 16 bytes from pattern table() -> you get an index into one of the 4 palletes for each pixel in the 8x8 tile
// 3. Read attribute byte -> choose palette(this determines the final color)
// 4. Combine pattern bits + pallete = final pixels
byte ppu_read_nametable_tile(byte nt_x, byte nt_y, byte coarse_x, byte coarse_y)
{
    uint16_t addr = 0x2000 | ((nt_y & 1) << 11) | ((nt_x & 1) << 10) | ((coarse_y & 0x1F) << 5) | (coarse_x & 0x1F);
    return ppu_read(addr);
}

void ppu_reset()
{
    control.reg = 0x0000;
    mask.reg = 0x0000;
    status.reg = status.reg & 0b10000000;
    PPU_BUFFER = 0;
    PPUSCROLL_latch = false;
    PPUADDR_latch = false;
    first_write = false;
    pause_after_frame = false;
    vaddr.reg = 0x0000;
    taddr.reg = 0x0000;
    bgs_pattern_l = 0x0000;
    bgs_pattern_h = 0x0000;
    bgs_attribute_l = 0x0000;
    bgs_attribute_h = 0x0000;
    for (int i = 0; i < 32; i++)
        pallete_table[i] = 0;
}


