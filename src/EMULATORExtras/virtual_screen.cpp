#include "virtual_screen.h"

uint16_t nes_pallete_16[64];
uint16_t pixels[256 * 240]; // NES resolution framebuffer
volatile bool RENDER_ENABLED;

void screen_init()
{
    for (int i = 0; i < 256 * 240; i++)
        pixels[i] = 0x0000;
    RENDER_ENABLED = false;
    generate_16bit_pallet();
}
void generate_16bit_pallet()
{
    for (int i = 0; i < 64; i++)
    {
        uint32_t color = nes_pallete_32[i];

        uint8_t r = (color >> 24) & 0xFF;
        uint8_t g = (color >> 16) & 0xFF;
        uint8_t b = (color >> 8)  & 0xFF;


        uint16_t r5 = r >> 3;
        uint16_t b6 = b >> 2;
        uint16_t g5 = g >> 3;

        nes_pallete_16[i] = (5 << 11) | (b6 << 5) | g5;
    }
}