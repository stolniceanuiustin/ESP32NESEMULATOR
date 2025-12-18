#ifndef VIRTUAL_SCREEN_H
#define VIRTUAL_SCREEN_H
#include <stdint.h>
#include <iostream>

/*
This file contains a virtual screen from which
the framebuffer will be created.
*/
const uint32_t nes_pallete_32[64] = {
    // First 16 colors (universal background + first 3 palettes)
    0x7C7C7CFF, 0x0000FCFF, 0x0000BCFF, 0x4428BCFF,
    0x940084FF, 0xA80020FF, 0xA81000FF, 0x881400FF,
    0x503000FF, 0x007800FF, 0x006800FF, 0x005800FF,
    0x004058FF, 0x000000FF, 0x000000FF, 0x000000FF,

    0xBCBCBCFF, 0x0078F8FF, 0x0058F8FF, 0x6844FCFF,
    0xD800CCFF, 0xE40058FF, 0xF83800FF, 0xE45C10FF,
    0xAC7C00FF, 0x00B800FF, 0x00A800FF, 0x00A844FF,
    0x008888FF, 0x000000FF, 0x000000FF, 0x000000FF,

    0xF8F8F8FF, 0x3CBCFCFF, 0x6888FCFF, 0x9878F8FF,
    0xF878F8FF, 0xF85898FF, 0xF87858FF, 0xFCA044FF,
    0xF8B800FF, 0xB8F818FF, 0x58D854FF, 0x58F898FF,
    0x00E8D8FF, 0x787878FF, 0x000000FF, 0x000000FF,

    0xFCFCFCFF, 0xA4E4FCFF, 0xB8B8F8FF, 0xD8B8F8FF,
    0xF8B8F8FF, 0xF8A4C0FF, 0xF0D0B0FF, 0xFCE0A8FF,
    0xF8D878FF, 0xD8F878FF, 0xB8F8B8FF, 0xB8F8D8FF,
    0x00FCFCFF, 0xF8D8F8FF, 0x000000FF, 0x000000FF};

extern uint16_t nes_pallete_16[64];
void generate_16bit_pallet(); // Convert 32-bit palette to 16-bit
void screen_init();
extern uint16_t pixels[256 * 240]; // NES resolution framebuffer
extern volatile bool RENDER_ENABLED;

void inline screen_set_pixel(uint16_t scanline, uint16_t dot, uint8_t color_index)
{
        // giving up on boundry checks so no time is wasted here!
        // In a mission critical system you might want to keep this
        
    // if (scanline >= 240 || dot >= 256 || color_index >= 64)
    //     return;

    //256 * scanline + dot is a one cycle instrution - Indexed Store
    pixels[256 * scanline + dot] = nes_pallete_16[color_index];
}


#endif