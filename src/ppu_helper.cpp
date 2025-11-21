#include "ppu.h"
#include <Arduino.h>

//Each 8x8 backgroud tile is encoded by 16 bytes
//For each pixel - 2 bit color value that idnex from pallete
//Normally, for CHRram, we should update this 
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

byte ppu_read_nametable_tile(byte nt_x, byte nt_y, byte coarse_x, byte coarse_y)
{
    //basically its like this: 
    //0x2000 | nt_y & 1
    
    uint16_t addr = 0x2000 | ((nt_y & 1) << 11) | ((nt_x & 1) << 10) | ((coarse_y & 0x1F) << 5) | (coarse_x & 0x1F);
    return ppu_read(addr);
}