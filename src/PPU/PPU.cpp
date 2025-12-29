#include "ppu.h"
#include "emulator_config.h"
#include "cpu.h"
#include <Arduino.h>
#include "cartridge.h"

static byte *nametable_ptrs[4];

void ppu_init()
{
    current_frame = 0;
    status.reg = 0b10100000;
    control.reg = 0x00;
    mask.reg = 0x00;
    OAMADDR = 0x00;
    OAMDATA = 0x00;
    odd_frame = false;
    // v = 0;
    scanline = 0;
    dots = 0;
    fine_x = 0;
    transparent_pixel_color = ppu_read(0x3F00) & 0x3F;

    generate_flip_byte_lt();

    for (int i = 0; i < 64; i++)
    {
        OAM[i].y = 0x00;
        OAM[i].x = 0x00;
        OAM[i].id = 0x00;
        OAM[i].attributes = 0x00;
    }
    for (int i = 0; i < 8; i++)
    {
        sprites_on_scanline[i] = {0, 0, 0, 0};
        sprite_cnt = 0;
        sp_pattern_h[i] = 0;
        sp_pattern_l[i] = 0;
    }

    if (mirroring == HORIZONTAL)
    {
        nametable_ptrs[0] = nametable[0];
        nametable_ptrs[1] = nametable[1];
        nametable_ptrs[2] = nametable[0];
        nametable_ptrs[3] = nametable[1];
    }
    else if (mirroring == VERTICAL)
    {
        nametable_ptrs[0] = nametable[0];
        nametable_ptrs[1] = nametable[0];
        nametable_ptrs[2] = nametable[1];
        nametable_ptrs[3] = nametable[1];
    }

    memset(nametable[0], 0, 1024);
    memset(nametable[1], 0, 1024);
    memset(pallete_table, 0, 32);
    tile_cache_initialized = false;
}

byte IRAM_ATTR ppu_read(uint16_t addr)
{
    addr &= 0x3FFF;
    if (addr >= 0x0000 && addr <= 0x1FFF) // reading in CHRrom(sprites usually)
    {
        // hardcoding mapper0
        return CHRrom[addr];
    }
    else if (addr >= 0x2000 && addr <= 0x3EFF) // reading in nametable, which is not read_only
    {
        // https://www.nesdev.org/wiki/Mirroring

        addr &= 0x0FFF;
        uint8_t nt_index = addr >> 10;   // top 2 bits: 0..3
        uint16_t offset = addr & 0x03FF; // lower 10 bits: offset inside NT
        return nametable_ptrs[nt_index][offset];
    }
    else if (addr >= 0x3F00) // reading in pallete table
    {
        addr &= 0x1F;
        if ((addr & 0x03) == 0 && (addr & 0x10) != 0)
        {
            addr &= 0x0F;
        }
        return pallete_table[addr] & (mask.grayscale ? 0x30 : 0x3F);
    }
    else
    {
        // The PPU should in no circumstance get here.
        return 0;
    }
}

void IRAM_ATTR ppu_write(uint16_t addr, uint8_t data)
{
    addr &= 0x3FFF;
    if (addr >= 0x2000 && addr <= 0x3EFF)
    {
        /*
            https://www.nesdev.org/wiki/Mirroring
        */
        addr &= 0x0FFF;
        uint8_t nt_index = addr >> 10;   // top 2 bits: 0..3
        uint16_t offset = addr & 0x03FF; // lower 10 bits: offset inside NT
        nametable_ptrs[nt_index][offset] = data;
        return;
    }
    else if (addr >= 0x3F00)
    {
        addr &= 0x001F;
        switch (addr)
        {
        case 0x10:
            addr = 0x00;
            break;
        case 0x14:
            addr = 0x04;
            break;
        case 0x18:
            addr = 0x08;
            break;
        case 0x1C:
            addr = 0x0C;
            break;
        }
        pallete_table[addr] = data;
        return;
    }
    else if (addr >= 0x0000 && addr <= 0x1FFF)
    {
        // This should just get ignored, as some games attempt to write here for some reason.
        // Mapper0 games dont write to this zone
        // For other mappers, the following should happen:
        // ppu_write(addr, data);
        return;
    }
}

// Here you can find all the registers
// https://www.nesdev.org/wiki/PPU_registers : Those are the registers used for communication between CPU AND PPU
// There are also internal registers for the PPU
// https://www.nesdev.org/wiki/PPU_scrolling#PPU_internal_registers
// Also i want to thank javidx9 so much for explaing the PPU inner workings
// https://www.youtube.com/watch?v=-THeUXqR3zY

byte IRAM_ATTR ppu_read_from_cpu(byte addr)
{
    byte data = 0x00;
    switch (addr)
    {
    case 0: // Control Register, Write Only
        break;
    case 1: // Mask Register, Write Only
        break;
    case 2:                                                           // Status Register, Read Only
        data = (status.reg & 0b11100000) | (PPU_BUFFER & 0b00011111); // last 5 bits of the last ppu bus transaction
        clear_vblank();
        PPUADDR_latch = false;
        return data;
    case 3: // OAM ADDR, Write Only
        break;
    case 4: // OAM DATA - returns sprite data
        return pOAM[OAMADDR];
        break;
    case 5: // Scroll, Write only
        break;
    case 6:    // PPU addr, write only
        break; // PPU Data, both read and write are allowed
    case 7:
        data = PPU_BUFFER;
        PPU_BUFFER = ppu_read(vaddr.reg);
        if (vaddr.reg >= 0x3F00)
            data = PPU_BUFFER;
        vaddr.reg += (control.increment_mode ? 32 : 1); // either vertical or horizontal jumps in nametable
        return data;
        break;
    }
    //}
    return data;
}

void IRAM_ATTR ppu_write_from_cpu(byte addr, byte data)
{
    switch (addr)
    {
    case 0: // Control register.
        control.reg = data;
        taddr.nametable_x = control.nametable_x;
        taddr.nametable_y = control.nametable_y;
        break;
    case 1: // MASK REGISTER
        mask.reg = data;
        break;
    case 2: // STATUS - read only
        break;
    case 3: // OAM ADDR - write only
        OAMADDR = data;
        return;
    case 4: // OAM DATA - WR
        pOAM[OAMADDR] = data;
        return;
    case 5: // Scroll Register - Shares the same latch as PPU addr
        if (PPUADDR_latch == false)
        {
            fine_x = data & 0x07;
            taddr.coarse_x = data >> 3;
            PPUADDR_latch = true;
        }
        else
        {
            taddr.fine_y = data & 0x07;
            taddr.coarse_y = data >> 3;
            PPUADDR_latch = false;
        }
        return;
    case 6: // PPU Addr - as the addr is 12 bits wide, we need to do two writes
        if (PPUADDR_latch == false)
        {
            taddr.reg = (uint16_t)((data & 0b00111111) << 8) | (taddr.reg & 0x00FF);
            PPUADDR_latch = true;
        }
        else
        {
            taddr.reg = (taddr.reg & 0xFF00) | data;
            vaddr = taddr;
            PPUADDR_latch = false;
        }
        return;
    case 7: // PPUDATA, Read Write
        ppu_write(vaddr.reg, data);
        vaddr.reg += (control.increment_mode ? 32 : 1);
    }
}

// Builds one scanline of background.
// Read the explanations in ppu_helper if you want to understand the process

loopy IRAM_ATTR build_background_scanline(int scanline_index, loopy vaddr_snapshot, byte fine_x_snapshot)
{
    if (!tile_cache_initialized)
        build_tile_cache();

    byte coarse_x = vaddr_snapshot.coarse_x;
    byte coarse_y = vaddr_snapshot.coarse_y;
    byte nt_x = vaddr_snapshot.nametable_x;
    byte nt_y = vaddr_snapshot.nametable_y;
    byte fine_y = vaddr_snapshot.fine_y;

    int32_t dest_x = -((int)fine_x_snapshot);

    // TODO: This works only for mapper0! If implementing any other mappers in the future, look at this
    for (int tile_i = 0; tile_i < 33; tile_i++) // we need 33 tiles to cover partial tiles
    {
        byte ntx = nt_x;
        byte tx = coarse_x + tile_i;
        // horizontal wrap
        if (tx >= 32)
        {
            tx -= 32;
            ntx = !ntx;
        }

        byte tile_id = ppu_read_nametable_tile(ntx, nt_y, tx, coarse_y);
        uint16_t pattern_index = tile_id | (control.pattern_background << 8);

        // copy pixels from tile_pixels into buffer
        for (int px = 0; px < 8; px++)
        {
            int out_x = dest_x + px;

            byte attr_coarse_x = tx >> 2;
            byte attr_coarse_y = coarse_y >> 2;

            // Attribute address math is kinda intimidating, the wiki explains it pretty well

            uint16_t attr_addr = 0x23C0 | (nt_y << 11) | (ntx << 10) | (attr_coarse_y << 3) | (attr_coarse_x);
            byte attr_byte = ppu_read(attr_addr);

            byte shift = ((coarse_y & 2) ? 4 : 0) + ((tx & 2) ? 2 : 0);
            byte pallete_high = (attr_byte >> shift) & 0x03;

            if (out_x >= 0 && out_x < 256)
            {
                // this only contains pallet index, no attribute data
                byte pixel = tile_pixels[pattern_index][fine_y][px];

                byte combined = (pallete_high << 2) | (pixel & 0x03);

                scanline_buffer[out_x] = combined;
            }
        }
        dest_x += 8;
    }

    loopy vaddr_next = vaddr_snapshot;
    return vaddr_next;
}

uint32_t start_cycles;
uint32_t end_cycles;

byte sprite_pixel[256] = {0};
byte sprite_palette[256] = {0};
byte sprite_priority[256] = {0};
bool sprite_zero_pixel[256] = {false};


void IRAM_ATTR ppu_render_scanline()
{
    if (scanline == -1)
    {
        clear_vblank();
        status.sprite_overflow = 0;
        status.sprite_zero_hit = 0;
        std::memset(sp_pattern_h, 0, 8);
        std::memset(sp_pattern_l, 0, 8);
        transfer_address_y();
    }
    else if (scanline >= 0 && scanline < 240)
    {
        std::memset(sprite_pixel, 0, 256);
        std::memset(sprite_palette, 0, 256);
        std::memset(sprite_priority, 0, 256);
        std::memset(sprite_zero_pixel, 0, 256);
        // On real hardware this is done at dot 257 but we do it at the beginning, doesn't seem to be an issue
        transfer_address_x();

        // BUILD BACKGROUND
        vaddr = build_background_scanline(scanline, vaddr, fine_x);

        // SPRITE PROCESS

        // Conceptually: The PPU goes through the Object Attribute Memory (OAM), which contain data for 64 sprites
        // For each sprite, it calculates the difference between the current scnaline and sprite's y coordinate
        // If the difference falls within the sprite's height, then we say this sprite is *active* for this scanline
        // The limit for one line is 8 sprites. If more than 8 are found we need to set the status.sprite_overflow flag
        // If the very first sprite in memory is found on the current like. we set sprite_zero_on_scanline (we use this for rendering effects)

        // SPRITE EVALUATION -> finding sprites for the line that is to be rendered
        sprite_cnt = 0;
        sprite_zero_on_scanline = false;
        // std::memset(sprites_on_scanline, 0xFF, 8 * sizeof(_OAM));
        init_sprites_on_scanline(); // This function is basically the commented memset but loop unrolled.
        std::memset(sp_pattern_l, 0, 8);
        std::memset(sp_pattern_h, 0, 8);
        uint32_t i = 0;
        while (i < 64)
        {
            int16_t diff = scanline - (int16_t)OAM[i].y;
            uint32_t sprite_height = control.sprite_size ? 16 : 8;

            if (diff >= 0 && diff < (int)sprite_height)
            {
                if (sprite_cnt < 8)
                {
                    if (i == 0)
                        sprite_zero_on_scanline = true;
                    memcpy(&sprites_on_scanline[sprite_cnt], &OAM[i], sizeof(_OAM));
                    sprite_cnt++;
                }
                else
                {
                    status.sprite_overflow = 1;
                    break;
                }
            }
            i++;
        }

        // DATA FETCHING
        // Once the 8 sprites are identified, the PPU fetches their specific tile data from the pattern tables(the same proccess as background)
        // If vertical flip attribute bit is set, the ppu just reverses the row indexing
        for (int i = 0; i < sprite_cnt; i++)
        {
            uint16_t addr_l = 0;
            byte attr = sprites_on_scanline[i].attributes;
            int diff_y = scanline - sprites_on_scanline[i].y;

            // this handles vertical flip
            if (attr & 0x80)
                diff_y = (control.sprite_size ? 15 : 7) - diff_y;
            if (!control.sprite_size) // 8x8 mode
            {
                addr_l = (control.pattern_sprite << 12) | (sprites_on_scanline[i].id << 4) | diff_y;
            }
            else // 8x16 mode
            {
                byte bank = sprites_on_scanline[i].id & 0x01;
                byte tile = sprites_on_scanline[i].id & 0xFE;
                if (diff_y >= 8)
                {
                    diff_y -= 8;
                    tile++;
                }
                addr_l = (bank << 12) | (tile << 4) | diff_y;
            }
            byte p_l = ppu_read(addr_l);
            byte p_h = ppu_read(addr_l + 8);

            if (attr & 0x40) // if horizontal flip
            {
                p_l = flip_byte[p_l];
                p_h = flip_byte[p_h];
            }
            sp_pattern_l[i] = p_l;
            sp_pattern_h[i] = p_h;

            int sx = sprites_on_scanline[i].x;
            byte pal = (sprites_on_scanline[i].attributes & 0x03) + 0x04;
            byte prio = (sprites_on_scanline[i].attributes >> 5) & 1;

            for (int px = 0; px < 8; px++)
            {
                int x = sx + px;
                if (x < 0 || x >= 256)
                    continue;

                int bit = 7 - px;
                byte pixel =
                    (((sp_pattern_h[i] >> bit) & 1) << 1) |
                    ((sp_pattern_l[i] >> bit) & 1);

                if (pixel != 0 && sprite_pixel[x] == 0)
                {
                    sprite_pixel[x] = pixel;
                    sprite_palette[x] = pal;
                    sprite_priority[x] = prio;

                    if (i == 0 && sprite_zero_on_scanline)
                        sprite_zero_pixel[x] = true;
                }
            }
        }

        // Pixel drawing. Here we draw all the pixels in a scanline, one by one.
        // One big optimisation would be for this to be made in one go.
        for (int x = 0; x < 256; x++)
        {
            byte bg_pixel = 0x00;
            byte bg_pallete = 0x00;
            if (mask.render_background && (mask.render_background_left || x >= 8))
            {
                byte combined = scanline_buffer[x];
                bg_pixel = combined & 0x03;
                bg_pallete = combined >> 2;
            }

            // Foreground logic(sprites)
            byte fg_pixel = 0;
            byte fg_pallete = 0;
            byte fg_priority = 0;
            bool sprite_zero_is_rendering = false;
            if (mask.render_sprites && (mask.render_sprites_left || x >= 8))
            {
                fg_pixel = sprite_pixel[x];
                fg_pallete = sprite_palette[x];
                fg_priority = sprite_priority[x];
                sprite_zero_is_rendering = sprite_zero_pixel[x];
            }

            byte pixel = 0;
            byte pallete = 0;
            if (fg_pixel == 0)
            {
                pixel = bg_pixel;
                pallete = bg_pallete;
            }
            else if (bg_pixel == 0)
            {
                pixel = fg_pixel;
                pallete = fg_pallete;
            }
            else
            {
                if (fg_priority == 0)
                {
                    pixel = fg_pixel;
                    pallete = fg_pallete;
                }
                else
                {
                    pixel = bg_pixel;
                    pallete = bg_pallete;
                }
                if (!status.sprite_zero_hit && sprite_zero_on_scanline && sprite_zero_is_rendering)
                {
                    if (mask.render_background && mask.render_sprites)
                    {
                        status.sprite_zero_hit = 1;
                    }
                }
            }

            byte color = 0;
            if (pixel != 0)
            {
                // Old way : color = ppu_read(0x3F00 + (pallete << 2) + pixel) & 0x3F;
                color = pallete_table[(pallete << 2) | pixel] & 0x3F;
            }
            else
                color = transparent_pixel_color;
            screen_set_pixel(scanline, x, color);
        }
        increment_scroll_y();
    }
    else if (scanline == 240)
    {
        // this does nothing
    }
    else if (scanline == 241)
    {
        set_vblank();
        transparent_pixel_color = ppu_read(0x3F00) & 0x3F;

        if (control.enable_nmi)
            enqueue_nmi();
    }

    scanline++;
    if (scanline > 261)
    {
        scanline = -1;
        RENDER_ENABLED = true;
    }
    return;
}

// HELPER FUNCTIONS
byte flip_byte_fn(byte x)
{
    byte aux = 0x00;
    for (int t = 7; t >= 0; t--)
    {
        aux = aux | ((x & 1) << t);
        x = x >> 1;
    }
    return aux;
}

void generate_flip_byte_lt()
{
    for (int i = 0; i < 256; i++)
    {
        flip_byte[i] = flip_byte_fn(i);
    }
}

inline void transfer_address_x()
{
    if (mask.render_background || mask.render_sprites)
    {
        vaddr.nametable_x = taddr.nametable_x;
        vaddr.coarse_x = taddr.coarse_x;
    }
}

inline void increment_scroll_x()
{
    if (mask.render_background)
    {
        if (vaddr.coarse_x == 31)
        {
            vaddr.coarse_x = 0;
            vaddr.nametable_x = ~vaddr.nametable_x;
        }
        else
            vaddr.coarse_x++;
    }
}

inline void increment_scroll_y()
{
    if (mask.render_background)
    {
        if (vaddr.fine_y < 7)
            vaddr.fine_y++;
        else
        {
            vaddr.fine_y = 0;
            if (vaddr.coarse_y == 29)
            {
                vaddr.coarse_y = 0;
                vaddr.nametable_y = ~vaddr.nametable_y;
            }
            else if (vaddr.coarse_y == 31)
            {
                vaddr.coarse_y = 0;
            }
            else
                vaddr.coarse_y++;
        }
    }
}

inline void transfer_address_y()
{
    if (mask.render_background | mask.render_sprites)
    {
        vaddr.nametable_y = taddr.nametable_y;
        vaddr.coarse_y = taddr.coarse_y;
        vaddr.fine_y = taddr.fine_y;
    }
};

inline void set_vblank()
{
    status.vertical_blank = 1;
}

inline void clear_vblank()
{
    status.vertical_blank = 0;
}

inline void init_sprites_on_scanline()
{
    uint32_t *pointer = (uint32_t *)sprites_on_scanline;
    pointer[0] = 0xFFFFFFFF;
    pointer[1] = 0xFFFFFFFF;
    pointer[2] = 0xFFFFFFFF;
    pointer[3] = 0xFFFFFFFF;
    pointer[4] = 0xFFFFFFFF;
    pointer[5] = 0xFFFFFFFF;
    pointer[6] = 0xFFFFFFFF;
    pointer[7] = 0xFFFFFFFF;
    // Not using memset because loop unrolling might be faster - Didn't profile this YET!
    // std::memset(sprites_on_scanline, 0xFF, 8 * sizeof(_OAM));
}