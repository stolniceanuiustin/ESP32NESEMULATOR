#include "ppu.h"
#include "emulator_config.h"
#include "cpu.h"
#include <Arduino.h>
//THIS FILE IS MAGIC. DO NOT CHANGE ANYTHING HERE.


//TODO: Remove all lambdas from this!


void clear_status_register(byte &x)
{
    x &= 0b00011111;
}

void PPU::reset()
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

//Called very much so inline needed
byte PPU::ppu_read(uint16_t addr, bool read_only)
{
    addr &= 0x3FFF;
    if (addr >= 0x0000 && addr <= 0x1FFF)
    {
        return cartridge->ppu_read(addr);
    }
    else if (addr >= 0x2000 && addr <= 0x3EFF)
    {
        /*
            https://www.nesdev.org/wiki/Mirroring
        */
        addr &= 0x0FFF;
        if (cartridge->mirroring == HORIZONTAL)
        {
            if (addr >= 0x0000 && addr <= 0x03FF)
                return nametable[0][addr & 0x03FF];
            if (addr >= 0x0400 && addr <= 0x07FF)
                return nametable[1][addr & 0x03FF];
            if (addr >= 0x0800 && addr <= 0x0BFF)
                return nametable[0][addr & 0x03FF];
            if (addr >= 0x0C00 && addr <= 0x0FFF)
                return nametable[1][addr & 0x03FF];
        }
        else if (cartridge->mirroring == VERTICAL)
        {
            if (addr >= 0x0000 && addr <= 0x03FF)
                return nametable[0][addr & 0x03FF];
            else if (addr >= 0x0400 && addr <= 0x07FF)
                return nametable[0][addr & 0x03FF];
            else if (addr >= 0x0800 && addr <= 0x0BFF)
                return nametable[1][addr & 0x03FF];
            else if (addr >= 0x0C00 && addr <= 0x0FFF)
                return nametable[1][addr & 0x03FF];
        }
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF)
    {
        addr &= 0x001F;
        // mirroring:
        if (addr == 0x10)
            addr = 0x00;
        else if (addr == 0x14)
            addr = 0x04;
        else if (addr == 0x18)
            addr = 0x08;
        else if (addr == 0x1C)
            addr = 0x0C;
        return pallete_table[addr] & (mask.grayscale ? 0x30 : 0x3F);
    }
    else
    {
        perror("PPU_read should not get here\n");
        return 0;
    }
}

//called very often so in line is needed
void PPU::ppu_write(uint16_t addr, uint8_t data)
{
   // Serial.println("PPU WRITE");
    addr &= 0x3FFF;
    if (addr >= 0x0000 && addr <= 0x1FFF)
    {
        cartridge->ppu_write(addr, data);
       // Serial.println("Writing to cartridge ROM. should not get here in normal circumstances");
    }
    else if (addr >= 0x2000 && addr <= 0x3EFF)
    {
        /*
            https://www.nesdev.org/wiki/Mirroring
        */
        // std::cout << "Writing to nametable memory\n";
        addr &= 0x0FFF;
        if (cartridge->mirroring == HORIZONTAL)
        {
            if (addr >= 0x0000 && addr <= 0x03FF)
                nametable[0][addr & 0x03FF] = data;
            if (addr >= 0x0400 && addr <= 0x07FF)
                nametable[1][addr & 0x03FF] = data;
            if (addr >= 0x0800 && addr <= 0x0BFF)
                nametable[0][addr & 0x03FF] = data;
            if (addr >= 0x0C00 && addr <= 0x0FFF)
                nametable[1][addr & 0x03FF] = data;
        }
        else if (cartridge->mirroring == VERTICAL)
        {
            if (addr >= 0x0000 && addr <= 0x07FF)
                nametable[0][addr & 0x03FF] = data;
            else if(addr >= 0x800 && addr <= 0xFFF)
                nametable[1][addr & 0x03FF] = data;
        }
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF)
    {
        //Serial.println("Writing to pallete table at addr: " + String(addr, HEX) + " Data: " + String(data, HEX));
        addr &= 0x001F;
        // mirroring:
        if (addr == 0x10)
            addr = 0x00;
        else if (addr == 0x14)
            addr = 0x04;
        else if (addr == 0x18)
            addr = 0x08;
        else if (addr == 0x1C)
            addr = 0x0C;
        pallete_table[addr] = data;
    }
}

byte PPU::read_from_cpu(byte addr, bool read_only)
{
    byte data = 0x00;
    if (read_only == true)
    {
        switch (addr)
        {
        case 0:
            data = control.reg;
            break;
        case 1:
            data = mask.reg;
            break;
        case 2:
            data = status.reg;
            break;
        case 3:
            data = OAMADDR;
            break;
        case 4:
            data = OAMDATA; // OAMDATA
            break;
        case 5:
            return -1;
        case 6:
            return -1; /// PPUADDR
        case 7:
            return -1;
        }
    }
    else if (read_only == false) // default mode
    {
        switch (addr)
        {
        case 0:
            break; // control not readable usually
        case 1:
            break; // mask not readable usually
        case 2:
            // TODO: CHECK THIS
            data = (status.reg & 0b11100000) | (PPU_BUFFER & 0b00011111); // last 5 bits of the last ppu bus transaction
            // i think clearing vblank is messing up with timing!
            clear_vblank();
            PPUADDR_latch = false;
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
            break;
        }
    }
    return data;
}

void PPU::write_from_cpu(byte addr, byte data)
{
    //Serial.println("PPU WRITE FROM CPU");
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

void PPU::execute()
{
    if (scanline >= -1 && scanline < 240)
    {
        if (scanline == 0 && dots == 0)
            dots = 1;

        if (scanline == -1 && dots == 1)
        {
            clear_vblank();
            status.sprite_overflow = 0;
            status.sprite_zero_hit = 0;
            for (int i = 0; i < 8; i++)
            {
                sp_pattern_h[i] = 0;
                sp_pattern_l[i] = 0;
            }
        }

        if ((dots >= 2 && dots < 258) || (dots >= 321 && dots < 338))
        {
            clock_shifters();
            switch ((dots - 1) % 8)
            {
            case 0:
                // Nametable fetch. The ppu has to fetch the next tile id from the nametable. Using the tile id it will fetch the bits from the nametable
                load_bg_shifters();
                // only 12 bit addressing for the ppu memory
                bg_next_tile_id = ppu_read(0x2000 | (vaddr.reg & 0x0FFF), false);

                break;
            case 2:
                // atribute table fetch
                // https://www.nesdev.org/wiki/PPU_scrolling#Tile_and_attribute_fetching
                bg_next_tile_attrib = ppu_read(0x23C0 | (vaddr.nametable_y << 11) | (vaddr.nametable_x << 10) | ((vaddr.coarse_y >> 2) << 3) | (vaddr.coarse_x >> 2));
                if (vaddr.coarse_y & 0x02)
                    bg_next_tile_attrib >>= 4;
                if (vaddr.coarse_x & 0x02)
                    bg_next_tile_attrib >>= 2;
                bg_next_tile_attrib &= 0x03;
                // todo: figure what those bits actually mean
                break;
            case 4:
                // BG lsbits : least significant bits. fetch lsb for background tile
                bg_next_tile_lsb = ppu_read((control.pattern_background << 12) + ((uint16_t)bg_next_tile_id << 4) + (vaddr.fine_y) + 0);
                break;
            case 6:
                bg_next_tile_msb = ppu_read((control.pattern_background << 12) + ((uint16_t)bg_next_tile_id << 4) + (vaddr.fine_y) + 8);
                break;
            case 7:
                increment_scroll_x();
                break;
            }
        }
        // on last visible dot increment scroll y
        if (dots == VISIBLE_DOTS)
        {
            increment_scroll_y();
        }
        if (dots == VISIBLE_DOTS + 1)
        {
            load_bg_shifters();
            transfer_address_x();
        }
        if (dots == 338 || dots == 340)
        {
            // does an unused memory fetch
            bg_next_tile_id = ppu_read(0x2000 | (vaddr.reg & 0x0FFF));
        }
        if (scanline == -1 && dots >= 280 && dots < 305)
        {
            transfer_address_y();
        }
        if (scanline >= 0 && dots == 257) // Do the sprite evaluation at the end of the scanline
        {
            //std::memset(sprites_on_scanline, 0xFF, 8*sizeof(_OAM));
            init_sprites_on_scanline();
            sprite_cnt = 0;
            for (int i = 0; i < 8; i++)
            {
                sp_pattern_h[i] = 0;
                sp_pattern_l[i] = 0;
            }
            byte i = 0;
            sprite_zero_on_scanline = false;
            uint32_t* dst = (uint32_t*)sprites_on_scanline;
            uint32_t* src = (uint32_t*)OAM;
            // count to nine in case sprite overflow needs to be set
            while (i < 64 && sprite_cnt < 9)
            {
                // if this difference is pozitive => we can draw
                int16_t diff = (int16_t)scanline - (int16_t)OAM[i].y;

                if (diff >= 0 && diff < (control.sprite_size ? 16 : 8))
                {
                    if (sprite_cnt < 8)
                    {
                        if (i == 0) // check for sprite 0
                        {
                            sprite_zero_on_scanline = true;
                        }
                        //memcpy(&sprites_on_scanline[sprite_cnt], &OAM[i], sizeof(_OAM));
                        dst[sprite_cnt] = src[i];
                        sprite_cnt++;
                    }
                }
                i++;
            }
            status.sprite_overflow = (sprite_cnt > 8);
        }
        
        if (dots == 340)
        {
            for (int i = 0; i < sprite_cnt; i++)
            {
                byte sp_pattern_bits_l, sp_pattern_bits_h;
                uint16_t sp_pattern_addr_l, sp_pattern_addr_h;
                if (!control.sprite_size) // 8 tall sprites
                {
                    if ((sprites_on_scanline[i].attributes & 0x80) == 0) // if nt using vertical flipping
                    {
                        sp_pattern_addr_l = (control.pattern_sprite << 12) | (sprites_on_scanline[i].id << 4) | (scanline - sprites_on_scanline[i].y);
                    }
                    else
                    {
                        sp_pattern_addr_l = (control.pattern_sprite << 12) | (sprites_on_scanline[i].id << 4) | (7 - (scanline - sprites_on_scanline[i].y));
                    }
                }
                else
                {                                                        // 16 tall sprites
                    if ((sprites_on_scanline[i].attributes & 0x80) == 0) // if not using vertical flipping
                    {
                        if (scanline - sprites_on_scanline[i].y < 8) // top half
                        {
                            sp_pattern_addr_l = ((sprites_on_scanline[i].id & 0x01) << 12) | ((sprites_on_scanline[i].id & 0xFE) << 4) | ((scanline - sprites_on_scanline[i].y) & 0x07);
                        }
                        else
                        {
                            sp_pattern_addr_l = ((sprites_on_scanline[i].id & 0x01) << 12) | (((sprites_on_scanline[i].id & 0xFE) + 1) << 4) | ((scanline - sprites_on_scanline[i].y) & 0x07);
                        }
                    }
                    else // if using vertical flipping
                    {
                        if (scanline - sprites_on_scanline[i].y < 8) // top half
                        {
                            sp_pattern_addr_l = ((sprites_on_scanline[i].id & 0x01) << 12) | ((sprites_on_scanline[i].id & 0xFE) << 4) | (7 - (scanline - sprites_on_scanline[i].y) & 0x07);
                        }
                        else
                        {
                            sp_pattern_addr_l = ((sprites_on_scanline[i].id & 0x01) << 12) | (((sprites_on_scanline[i].id & 0xFE) + 1) << 4) | (7 - (scanline - sprites_on_scanline[i].y) & 0x07);
                        }
                    }
                }
                sp_pattern_addr_h = sp_pattern_addr_l + 8;
                sp_pattern_bits_h = ppu_read(sp_pattern_addr_h);
                sp_pattern_bits_l = ppu_read(sp_pattern_addr_l);

                if (sprites_on_scanline[i].attributes & 0x40) // if using ohorizontal flipping, the bytes of the pattern are flipped
                {
                    sp_pattern_bits_l = flip_byte[sp_pattern_bits_l];
                    sp_pattern_bits_h = flip_byte[sp_pattern_bits_h];
                }
                sp_pattern_l[i] = sp_pattern_bits_l;
                sp_pattern_h[i] = sp_pattern_bits_h;
            }
        }
    }
    if (scanline == VISIBLE_SCANLINES) // post render scanline, nothing happens here
        ;
    if (scanline > VISIBLE_SCANLINES && scanline < FRAME_END_SCANLINE)
    {
        if (scanline == VISIBLE_SCANLINES + 1 && dots == 1)
        {
            set_vblank();
            if (control.enable_nmi == 1)
            {
                cpu.enqueue_nmi();
            }
        }
    }

    byte bg_pixel = 0x00;
    byte bg_pallete = 0x00;
    if (mask.render_background)
    {
        // TODO: Check this, need to figure out scrolling, ignore for now
        uint16_t bit_mux = 0x8000 >> fine_x;

        byte p0_pixel = (bgs_pattern_l & bit_mux) > 0;
        byte p1_pixel = (bgs_pattern_h & bit_mux) > 0;
        bg_pixel = (p1_pixel << 1) | p0_pixel;

        byte bg_pallete0 = (bgs_attribute_l & bit_mux) > 0;
        byte bg_pallete1 = (bgs_attribute_h & bit_mux) > 0;
        bg_pallete = (bg_pallete1 << 1) | bg_pallete0;
    }

    byte fg_pixel = 0;
    byte fg_pallete = 0;
    byte fg_priority = 0;

    if (mask.render_sprites)
    {
        sprite_zero_is_rendering = false;
        for (int i = 0; i < sprite_cnt; i++)
        {
            if (sprites_on_scanline[i].x == 0)
            {
                byte fg_pixel_l = (sp_pattern_l[i] & 0x80) > 0;
                byte fg_pixel_h = (sp_pattern_h[i] & 0x80) > 0;
                fg_pixel = (fg_pixel_h << 1) | fg_pixel_l;

                fg_pallete = (sprites_on_scanline[i].attributes & 0b00000011) + 0x04;
                fg_priority = (sprites_on_scanline[i].attributes & 0b00100000) == 0; // 0 = in front of bg, 1 = behind bg
                if (fg_pixel != 0)
                {
                    if (i == 0) // if sprite 0 then enable sprite_zero_is_rendering
                        sprite_zero_is_rendering = true;
                    break;
                }
            }
        }
    }
    byte pixel = 0;
    byte pallete = 0;
    if (bg_pixel == 0 && fg_pixel == 0) // bots pixels are transparent
    {
        pixel = 0;
        pallete = 0;
    }
    else if (bg_pixel == 0 && fg_pixel > 0) // background is transparent, fg is visible, obviously show foreground
    {
        pixel = fg_pixel;
        pallete = fg_pallete;
    }
    else if (bg_pixel > 0 && fg_pixel == 0)
    {
        pixel = bg_pixel;
        pallete = bg_pallete;
    }
    else if (bg_pixel > 0 && fg_pixel > 0)
    {
        if (fg_priority)
        {
            pixel = fg_pixel;
            pallete = fg_pallete;
        }
        else
        {
            pixel = bg_pixel;
            pallete = bg_pallete;
        }
        if (sprite_zero_on_scanline == true && sprite_zero_is_rendering == true)
        {
            if (mask.render_background && mask.render_sprites)
            {
                if (~(mask.render_background_left | mask.render_sprites_left))
                {
                    if (dots >= 9 && dots < 258)
                        status.sprite_zero_hit = 1;
                }
                else
                {
                    if (dots >= 1 && dots < 258)
                        status.sprite_zero_hit = 1;
                }
            }
        }
    }

    if (scanline >= 0 && scanline < 240 && dots > 0 && dots <= 256)
    {
        byte color = 0;
        if (pixel != 0)
        {
            color = ppu_read(0x3F00 + (pallete << 2) + pixel, false) & 0x3F;
        }
        else
            color = ppu_read(0x3F00, false) & 0x3F;
        screen.set_pixel(scanline, dots - 1, color);
    }

    dots++;
    if (dots > LAST_SCANLINE_DOT)
    {
        dots = 0;
        scanline++;
        if (scanline > FRAME_END_SCANLINE)
        {
            scanline = -1;
            screen.RENDER_ENABLED = true;
        }
    }
}



//HELPER FUNCTIONS 
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

void PPU::generate_flip_byte_lt()
{
    for (int i = 0; i < 255; i++)
    {
        flip_byte[i] = flip_byte_fn(i);
    }
}

void PPU::clock_shifters()
{
    if (mask.render_background)
    {
        bgs_pattern_l <<= 1;
        bgs_pattern_h <<= 1;
        bgs_attribute_h <<= 1;
        bgs_attribute_l <<= 1;
    }
    if (mask.render_sprites && dots >= 1 && dots < 258)
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

inline void PPU::load_bg_shifters()
{
    bgs_pattern_l = (bgs_pattern_l & 0xFF00) | bg_next_tile_lsb;
    bgs_pattern_h = (bgs_pattern_h & 0xFF00) | bg_next_tile_msb;
    bgs_attribute_l = (bgs_attribute_l & 0xFF00) | ((bg_next_tile_attrib & 0b01) ? 0xFF : 0x00); // The lsbit of the attribute is mirrored 8 times
    bgs_attribute_h = (bgs_attribute_h & 0xFF00) | ((bg_next_tile_attrib & 0b10) ? 0xFF : 0x00);
}

inline void PPU::transfer_address_x()
{
    if (mask.render_background || mask.render_sprites)
    {
        vaddr.nametable_x = taddr.nametable_x;
        vaddr.coarse_x = taddr.coarse_x;
    }
}

inline void PPU::increment_scroll_x()
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

inline void PPU::increment_scroll_y()
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

inline void PPU::transfer_address_y()
{
    if (mask.render_background | mask.render_sprites)
    {
        vaddr.nametable_y = taddr.nametable_y;
        vaddr.coarse_y = taddr.coarse_y;
        vaddr.fine_y = taddr.fine_y;
    }
};

inline void PPU::set_vblank()
{
    status.vertical_blank = 1;
}

inline void PPU::clear_vblank()
{
    status.vertical_blank = 0;
}

inline void PPU::init_sprites_on_scanline()
{
    uint32_t* pointer = (uint32_t *)sprites_on_scanline;
    pointer[0] = 0xFFFFFFFF;
    pointer[1] = 0xFFFFFFFF;
    pointer[2] = 0xFFFFFFFF;
    pointer[3] = 0xFFFFFFFF;
    pointer[4] = 0xFFFFFFFF;
    pointer[5] = 0xFFFFFFFF;
    pointer[6] = 0xFFFFFFFF;
    pointer[7] = 0xFFFFFFFF; 
    //std::memset(sprites_on_scanline, 0xFF, 8 * sizeof(_OAM));
}