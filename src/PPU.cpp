#include "ppu.h"
#include "emulator_config.h"
#include "cpu.h"
#include <Arduino.h>
#include "cartridge.h"
// THIS FILE IS MAGIC. DO NOT CHANGE ANYTHING HERE.

// TODO: Remove all lambdas from this!

byte *nametable_ptrs[4];

void inline clear_status_register(byte &x)
{
    x &= 0b00011111;
}

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

// Called very much so inline needed
byte IRAM_ATTR ppu_read(uint16_t addr /*,bool read_only*/)
{
    addr &= 0x3FFF;
    if (addr >= 0x0000 && addr <= 0x1FFF) // reading in CHRrom(sprites usually)
    {
        // hardcoding mapper0
        return CHRrom[addr];
    }
    else if (addr >= 0x2000 && addr <= 0x3EFF) // reading in nametable
    {
        // https://www.nesdev.org/wiki/Mirroring

        addr &= 0x0FFF;
        uint8_t nt_index = addr >> 10;   // top 2 bits: 0..3
        uint16_t offset = addr & 0x03FF; // lower 10 bits: offset inside NT
        return nametable_ptrs[nt_index][offset];
    }
    else if (addr >= 0x3F00 /*&& addr <= 0x3FFF*/) // reading in pallete table
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
        perror("PPU_read should not get here\n");
        return 0;
    }
}

// called very often so in line is needed
void IRAM_ATTR ppu_write(uint16_t addr, uint8_t data)
{
    // Serial.println("PPU WRITE");
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
    }
    else if (addr >= 0x3F00 /*&& addr <= 0x3FFF*/)
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
    }
    else if (addr >= 0x0000 && addr <= 0x1FFF)
    {
        // This should just get ignored, as some games attempt to write here for some reason.
        // Mapper0 games dont write to this zone
        // ppu_write(addr, data);
        //Serial.println("========Writing to cartridge ROM. should not get here in normal circumstances!!!!=======");
    }
}

byte IRAM_ATTR ppu_read_from_cpu(byte addr)
{
    byte data = 0x00;
    switch (addr)
    {
    case 0:
        break; // control not readable usually
    case 1:
        break; // mask not readable usually
    case 2:
        // TODO: CHECK THIS
        data = (status.reg & 0b11100000) | (PPU_BUFFER & 0b00011111); // last 5 bits of the last ppu bus transaction
        // if (status.vertical_blank) {
        //     Serial.printf("CPU reading $2002. Reg value: 0x%02X\n", status.reg);
        // }   
        // i think clearing vblank is messing up with timing!
        clear_vblank();
        PPUADDR_latch = false;
        return data;
    case 3:
        // oam addr, write only
        break;
    case 4:
        return pOAM[OAMADDR];
        break;
    case 5:
        // scroll is not readable
        break;
    case 6:
        // ppu address is not readable
        break;
    case 7: // reads from memory but it is delayed one cycle
        data = PPU_BUFFER;
        PPU_BUFFER = ppu_read(vaddr.reg);
        if (vaddr.reg >= 0x3F00)
            data = PPU_BUFFER;                          // separate memory on the ppu
        vaddr.reg += (control.increment_mode ? 32 : 1); // either vertical or horizontal jumps in nametable
        return data;
        break;
    }
    //}
    return data;
}

void IRAM_ATTR ppu_write_from_cpu(byte addr, byte data)
{
    // Serial.println("PPU WRITE FROM CPU");
    switch (addr)
    {
    case 0:
        control.reg = data;
        taddr.nametable_x = control.nametable_x;
        taddr.nametable_y = control.nametable_y;
        break;
    case 1:
        mask.reg = data;
        break;
    case 2: // STATUS - read only
        break;
    case 3:
        OAMADDR = data;
        return;
    case 4:
        pOAM[OAMADDR] = data;
        return; // OAMDATA
    case 5:     // Scroll Register
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
    case 6:
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
    case 7:
        ppu_write(vaddr.reg, data); // PPUDATA
        vaddr.reg += (control.increment_mode ? 32 : 1);
    }
}

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
            if (out_x >= 0 && out_x < 256)
            {
                // this only contains pallet index, no attribute data
                byte pixel = tile_pixels[pattern_index][fine_y][px];

                byte attr_coarse_x = tx >> 2;
                byte attr_coarse_y = coarse_y >> 2;
                // TODO: maybe bitwise nt_y and tx with 1?
                // ppu_read(0x23C0 | (vaddr.nametable_y << 11) | (vaddr.nametable_x << 10) | ((vaddr.coarse_y >> 2) << 3) | (vaddr.coarse_x >> 2));
                uint16_t attr_addr = 0x23C0 | (nt_y << 11) | (ntx << 10) | (attr_coarse_y << 3) | (attr_coarse_x);
                byte attr_byte = ppu_read(attr_addr);

                byte shift = ((coarse_y & 2) ? 4 : 0) + ((tx & 2) ? 2 : 0);
                byte pallete_high = (attr_byte >> shift) & 0x03;

                byte combined = (pallete_high << 2) | (pixel & 0x03);

                scanline_buffer[out_x] = combined;
            }
        }
        dest_x += 8;
    }

    loopy vaddr_next = vaddr_snapshot;

    // this is what happpens after 32 increments
    //  for (int i = 0; i < 32; ++i) {
    //      if (vaddr_next.coarse_x == 31) {
    //          vaddr_next.coarse_x = 0;
    //          vaddr_next.nametable_x = !vaddr_next.nametable_x;
    //      } else {
    //          vaddr_next.coarse_x++;
    //      }
    //  }

    return vaddr_next;
}

uint32_t start_cycles;
uint32_t end_cycles;

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
        //Serial.printf("Scanline nr: %d\n", scanline);
        // ON real hardware this is done at dot 257 but we do it at the beginning
        transfer_address_x();
        vaddr = build_background_scanline(scanline, vaddr, fine_x);

        // start_cycles = xthal_get_ccount();
        //  SPRITE EVALUATION
        sprite_cnt = 0;
        sprite_zero_on_scanline = false;
        std::memset(sprites_on_scanline, 0xFF, 8 * sizeof(_OAM));
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

        //===DATA FETCHING=== (get the sprites from memory based on what we done before)
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
            else
            {
                // maybe spliting these into multiple variables will make it easier for the
                // compiler to optimize(fingers crossed!)
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
            if (attr & 0x40)
            {
                p_l = flip_byte[p_l];
                p_h = flip_byte[p_h];
            }
            sp_pattern_l[i] = p_l;
            sp_pattern_h[i] = p_h;
        }

        for (int x = 0; x < 256; x++)
        {
            byte bg_pixel = 0x00;
            byte bg_pallete = 0x00;
            if (mask.render_background)
            {
                if (mask.render_background_left || x >= 8)
                {
                    byte combined = scanline_buffer[x];
                    bg_pixel = combined & 0x03;
                    bg_pallete = combined >> 2;
                }
            }

            // Foreground logic(sprites)
            byte fg_pixel = 0;
            byte fg_pallete = 0;
            byte fg_priority = 0;
            bool sprite_zero_is_rendering = false;
            if (mask.render_sprites)
            {
                if (mask.render_sprites_left || x >= 8)
                {
                    for (int i = 0; i < sprite_cnt; i++)
                    {
                        int sx = sprites_on_scanline[i].x;
                        if (x >= sx && x < sx + 8)
                        {
                            int diff_x = x - sx;

                            int bit = 7 - diff_x;

                            byte pixel_bit = ((sp_pattern_h[i] >> bit) & 1) << 1 | ((sp_pattern_l[i] >> bit) & 1);

                            if (pixel_bit != 0)
                            {
                                fg_pixel = pixel_bit;
                                fg_pallete = (sprites_on_scanline[i].attributes & 0x03) + 0x04;
                                fg_priority = (sprites_on_scanline[i].attributes & 0x20) >> 5;

                                if (i == 0 && sprite_zero_on_scanline)
                                    sprite_zero_is_rendering = true;
                            }
                        }
                    }
                }
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
                color = ppu_read(0x3F00 + (pallete << 2) + pixel) & 0x3F;
            }
            else
                color = transparent_pixel_color;
            screen_set_pixel(scanline, x, color);
        }
        increment_scroll_y();
        // end_cycles = xthal_get_ccount();
        // Serial.printf("Scanline rendering took %d cycles!", end_cycles - start_cycles);
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
void IRAM_ATTR ppu_execute()
{
    if (scanline == -1)
    {
        if (dots == 1)
        {
            clear_vblank();
            status.sprite_overflow = 0;
            status.sprite_zero_hit = 0;
            std::memset(sp_pattern_h, 0, 8);
            std::memset(sp_pattern_l, 0, 8);
        }
    }
    if (scanline >= 0 && scanline < 240)
    {
        if (dots == 1)
        {
            transfer_address_x();
            vaddr = build_background_scanline(scanline, vaddr, fine_x);
        }

        if (dots == 256)
        {
            // start_cycles = xthal_get_ccount();
            //  SPRITE EVALUATION
            sprite_cnt = 0;
            sprite_zero_on_scanline = false;
            std::memset(sprites_on_scanline, 0xFF, 8 * sizeof(_OAM));
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

            //===DATA FETCHING=== (get the sprites from memory based on what we done before)
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
                else
                {
                    // maybe spliting these into multiple variables will make it easier for the
                    // compiler to optimize(fingers crossed!)
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
                if (attr & 0x40)
                {
                    p_l = flip_byte[p_l];
                    p_h = flip_byte[p_h];
                }
                sp_pattern_l[i] = p_l;
                sp_pattern_h[i] = p_h;
            }

            for (int x = 0; x < 256; x++)
            {
                byte bg_pixel = 0x00;
                byte bg_pallete = 0x00;
                if (mask.render_background)
                {
                    if (mask.render_background_left || x >= 8)
                    {
                        byte combined = scanline_buffer[x];
                        bg_pixel = combined & 0x03;
                        bg_pallete = combined >> 2;
                    }
                }

                // Foreground logic(sprites)
                byte fg_pixel = 0;
                byte fg_pallete = 0;
                byte fg_priority = 0;
                bool sprite_zero_is_rendering = false;
                if (mask.render_sprites)
                {
                    if (mask.render_sprites_left || x >= 8)
                    {
                        for (int i = 0; i < sprite_cnt; i++)
                        {
                            int sx = sprites_on_scanline[i].x;
                            if (x >= sx && x < sx + 8)
                            {
                                int diff_x = x - sx;

                                int bit = 7 - diff_x;

                                byte pixel_bit = ((sp_pattern_h[i] >> bit) & 1) << 1 | ((sp_pattern_l[i] >> bit) & 1);

                                if (pixel_bit != 0)
                                {
                                    fg_pixel = pixel_bit;
                                    fg_pallete = (sprites_on_scanline[i].attributes & 0x03) + 0x04;
                                    fg_priority = (sprites_on_scanline[i].attributes & 0x20) >> 5;

                                    if (i == 0 && sprite_zero_on_scanline)
                                        sprite_zero_is_rendering = true;
                                }
                            }
                        }
                    }
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
                    color = ppu_read(0x3F00 + (pallete << 2) + pixel) & 0x3F;
                }
                else
                    color = transparent_pixel_color;
                screen_set_pixel(scanline, x, color);
            }
            increment_scroll_y();
            // end_cycles = xthal_get_ccount();
            // Serial.printf("Scanline rendering took %d cycles!", end_cycles - start_cycles);
        }
    }
    if (scanline == 241 && dots == 1)
    {
        set_vblank();
        transparent_pixel_color = ppu_read(0x3F00) & 0x3F;
        if (control.enable_nmi)
        {
            enqueue_nmi();
        }
    }

    dots++;
    if (dots > 340)
    {
        dots = 0;
        scanline++;
        if (scanline > 261)
        {
            scanline = -1;
            RENDER_ENABLED = true;
        }
    }
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

// No longer needed
void inline IRAM_ATTR clock_shifters()
{
    // if (mask.render_background)
    // {
    //     bgs_pattern_l <<= 1;
    //     bgs_pattern_h <<= 1;
    //     bgs_attribute_h <<= 1;
    //     bgs_attribute_l <<= 1;
    // }
    if (mask.render_sprites /*&& dots >= 1 && dots < 258*/)
    {
        for (int i = 0; i < sprite_cnt; i++)
        {
            if (sprites_on_scanline[i].x > 0)
                sprites_on_scanline[i].x--;
            else
            {
                sp_pattern_l[i] <<= 1;
                sp_pattern_h[i] <<= 1;
            }
        }
    }
}

inline void load_bg_shifters()
{
    bgs_pattern_l = (bgs_pattern_l & 0xFF00) | bg_next_tile_lsb;
    bgs_pattern_h = (bgs_pattern_h & 0xFF00) | bg_next_tile_msb;
    bgs_attribute_l = (bgs_attribute_l & 0xFF00) | ((bg_next_tile_attrib & 0b01) ? 0xFF : 0x00); // The lsbit of the attribute is mirrored 8 times
    bgs_attribute_h = (bgs_attribute_h & 0xFF00) | ((bg_next_tile_attrib & 0b10) ? 0xFF : 0x00);
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
    // std::memset(sprites_on_scanline, 0xFF, 8 * sizeof(_OAM));
}