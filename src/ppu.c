#include "ppu.h"
#include "bus.h"


void ppu_tick(PPU *ppu, Bus *bus, uint32_t cycles)
{
    while (cycles > 0) {

        uint16_t next_event;

        switch (ppu->mode) {
            case PPU_MODE_OAM:
                next_event = 80;
                break;

            case PPU_MODE_DRAW:
                next_event = 252;     // 80 + 172
                break;

            case PPU_MODE_HBLANK:
                next_event = 456;
                break;

            case PPU_MODE_VBLANK:
                next_event = 456;
                break;
        }

        uint16_t remaining = next_event - ppu->dot;

        if (cycles < remaining) {
            ppu->dot += cycles;
            return;
        }

        ppu->dot += remaining;
        cycles -= remaining;
        ppu_event(ppu, bus);
    }
}
uint32_t dmg_color(uint8_t shade)
{
    switch (shade) {
        case 0:
            return 0xFFFFFFFF;

        case 1:
            return 0xFFAAAAAA;

        case 2:
            return 0xFF555555;

        case 3:
            return 0xFF000000;

        default:
            return 0xFFFF00FF;
    }
}

void ppu_event(PPU *ppu, Bus *bus)
{
    switch (ppu->mode) {

        case PPU_MODE_OAM:
            ppu->mode = PPU_MODE_DRAW;
            update_stat(bus);
            break;

        case PPU_MODE_DRAW:
            /*
             * Render the entire scanline here.
             */
            render_scanline(ppu, bus);

            ppu->mode = PPU_MODE_HBLANK;
            update_stat(bus);
            break;

        case PPU_MODE_HBLANK:
            ppu->dot = 0;
            ppu->ly++;

            if (ppu->ly == 144) {
                ppu->mode = PPU_MODE_VBLANK;

                /*
                 * Request VBlank interrupt.
                 */
                bus->IF |= 1 << 0;

                ppu->render_frame = 1;
            }
            else {
                ppu->mode = PPU_MODE_OAM;
            }
            update_stat(bus);
            break;

        case PPU_MODE_VBLANK:
            ppu->dot = 0;
            ppu->ly++;

            if (ppu->ly > 153) {
                ppu->ly = 0;
                ppu->mode = PPU_MODE_OAM;
            }
            update_stat(bus);
            break;
    }
}
void update_stat(Bus *bus)
{
    PPU *ppu = &bus->ppu;

    /*
     * Clear mode bits 0-1.
     */
    ppu->stat &= ~0x03;

    /*
     * PPUMode values deliberately match STAT's mode values:
     *
     * 0 = HBlank
     * 1 = VBlank
     * 2 = OAM
     * 3 = Drawing
     */
    ppu->stat |= ppu->mode & 0x03;

    /*
     * Update LY == LYC coincidence flag.
     */
    if (ppu->ly == ppu->lyc) {
        ppu->stat |= (1 << 2);
    }
    else {
        ppu->stat &= ~(1 << 2);
    }

    /*
     * Determine whether a STAT interrupt condition is active.
     */
    uint8_t stat_irq = 0;

    switch (ppu->mode) {
        case PPU_MODE_HBLANK:
            if (ppu->stat & (1 << 3))
                stat_irq = 1;
            break;

        case PPU_MODE_VBLANK:
            if (ppu->stat & (1 << 4))
                stat_irq = 1;
            break;

        case PPU_MODE_OAM:
            if (ppu->stat & (1 << 5))
                stat_irq = 1;
            break;

        case PPU_MODE_DRAW:
            /*
             * There is no Mode 3 STAT interrupt.
             */
            break;
    }

    /*
     * LYC == LY interrupt.
     */
    if ((ppu->ly == ppu->lyc) &&
        (ppu->stat & (1 << 6))) {
        stat_irq = 1;
    }

    if (stat_irq) {
        bus->IF |= (1 << 1);   // LCD STAT interrupt
    }
}
void render_scanline(PPU *ppu, Bus *bus)
{
    if (ppu->ly >= 144)
        return;

    /* Background */
    if (ppu->lcdc & (1 << 0)) {
        render_background_line(ppu, bus);
    }
    else {
        for (int x = 0; x < 160; x++) {
            ppu->bg_color_id[x] = 0;
            ppu->framebuffer[ppu->ly * 160 + x] = dmg_color(0);
        }
    }

    /* Window: LCDC bit 5 */
    if (ppu->lcdc & (1 << 5)) {
        render_window_line(ppu, bus);
    }

    /* Sprites: LCDC bit 1 */
    if (ppu->lcdc & (1 << 1)) {
        render_sprite_line(ppu, bus);
    }
}
void render_background_line(PPU *ppu, Bus *bus)
{
    uint8_t ly = ppu->ly;

    /*
     * LCDC bit 3:
     *   0 -> BG tile map at 0x9800
     *   1 -> BG tile map at 0x9C00
     */
    uint16_t tile_map_base =
        (ppu->lcdc & (1 << 3))
        ? 0x9C00
        : 0x9800;

    /*
     * LCDC bit 4:
     *   1 -> tile data 0x8000-0x8FFF, unsigned tile IDs
     *   0 -> tile data 0x8800-0x97FF, signed tile IDs
     */
    uint8_t unsigned_tile_ids =
        (ppu->lcdc & (1 << 4)) != 0;

    uint8_t bg_y = ppu->scy + ly;

    /*
     * Which tile row in the 32x32 BG map?
     */
    uint8_t tile_row = bg_y >> 3;

    /*
     * Which pixel row inside the tile?
     */
    uint8_t pixel_y = bg_y & 0x07;

    for (uint16_t screen_x = 0; screen_x < 160; screen_x++) {

        uint8_t bg_x = ppu->scx + screen_x;

        uint8_t tile_col = bg_x >> 3;
        uint8_t pixel_x = bg_x & 0x07;

        uint16_t tile_map_addr =
            tile_map_base +
            ((uint16_t)tile_row * 32) +
            tile_col;

        uint8_t tile_id =
            read_memory_8(bus, tile_map_addr);

        uint16_t tile_addr;

        if (unsigned_tile_ids) {
            /*
             * Tile IDs 0..255
             *
             * 0x8000 + tile_id * 16
             */
            tile_addr =
                0x8000 +
                ((uint16_t)tile_id * 16);
        }
        else {
            /*
             * Signed addressing mode.
             *
             * tile ID interpreted as int8_t around 0x9000.
             */
            int8_t signed_id = (int8_t)tile_id;

            tile_addr =
                0x9000 +
                ((int16_t)signed_id * 16);
        }

        /*
         * Each tile row is 2 bytes.
         */
        uint16_t row_addr =
            tile_addr +
            ((uint16_t)pixel_y * 2);

        uint8_t low =
            read_memory_8(bus, row_addr);

        uint8_t high =
            read_memory_8(bus, row_addr + 1);

        /*
         * Left-most tile pixel is bit 7.
         */
        uint8_t bit = 7 - pixel_x;

        uint8_t color_id =
            (((high >> bit) & 1) << 1) |
             ((low  >> bit) & 1);

        ppu->bg_color_id[screen_x] = color_id;
        /*
         * Convert the 2-bit BG colour through BGP.
         */
        uint8_t shade =
            (ppu->bgp >> (color_id * 2)) & 0x03;
        ppu->framebuffer[
            (uint16_t)ly * 160 + screen_x
        ] = dmg_color(shade);
    }
}
void render_window_line(PPU *ppu, Bus *bus)
{
    /*
     * Window isn't visible yet.
     */
    if (ppu->ly < ppu->wy)
        return;

    /*
     * WX >= 167 means the window is completely off-screen.
     */
    if (ppu->wx >= 167)
        return;

    /*
     * LCDC bit 6 selects the window tile map:
     *
     * 0 -> 0x9800
     * 1 -> 0x9C00
     */
    uint16_t tile_map_base =
        (ppu->lcdc & (1 << 6))
        ? 0x9C00
        : 0x9800;

    /*
     * Same tile-data addressing mode as the BG.
     * LCDC bit 4.
     */
    uint8_t unsigned_tile_ids =
        (ppu->lcdc & (1 << 4)) != 0;

    /*
     * Window coordinates.
     */
    int window_x = (int)ppu->wx - 7;

    uint8_t window_y =
        ppu->ly - ppu->wy;

    uint8_t tile_row =
        window_y >> 3;

    uint8_t pixel_y =
        window_y & 0x07;

    for (int screen_x = 0; screen_x < 160; screen_x++) {

        /*
         * Pixel lies to the left of the window.
         */
        if (screen_x < window_x)
            continue;

        int wx_pixel =
            screen_x - window_x;

        uint8_t tile_col =
            wx_pixel >> 3;

        uint8_t pixel_x =
            wx_pixel & 0x07;

        uint16_t tile_map_addr =
            tile_map_base +
            ((uint16_t)tile_row * 32) +
            tile_col;

        uint8_t tile_id =
            read_memory_8(bus, tile_map_addr);

        uint16_t tile_addr;

        if (unsigned_tile_ids) {
            tile_addr =
                0x8000 +
                ((uint16_t)tile_id * 16);
        }
        else {
            tile_addr =
                0x9000 +
                ((int16_t)(int8_t)tile_id * 16);
        }

        uint16_t row_addr =
            tile_addr +
            ((uint16_t)pixel_y * 2);

        uint8_t low =
            read_memory_8(bus, row_addr);

        uint8_t high =
            read_memory_8(bus, row_addr + 1);

        uint8_t bit =
            7 - pixel_x;

        uint8_t color_id =
            (((high >> bit) & 1) << 1) |
             ((low  >> bit) & 1);

        /*
         * Window counts as BG for sprite priority.
         */
        ppu->bg_color_id[screen_x] =
            color_id;

        uint8_t shade =
            (ppu->bgp >> (color_id * 2)) & 0x03;

        ppu->framebuffer[
            (uint16_t)ppu->ly * 160 + screen_x
        ] = dmg_color(shade);
    }
}
void render_sprite_line(PPU *ppu, Bus *bus)
{
    uint8_t sprite_height =
        (ppu->lcdc & (1 << 2))
        ? 16
        : 8;

    /*
     * Hardware limits each scanline to 10 sprites.
     */
    uint8_t sprites_found = 0;

    /*
     * Process in OAM order.
     *
     * This is acceptable for a first DMG renderer.
     */
    for (int i = 0; i < 40; i++) {

        uint16_t oam =
            0xFE00 + i * 4;

        uint8_t raw_y =
            read_memory_8(bus, oam + 0);

        uint8_t raw_x =
            read_memory_8(bus, oam + 1);

        uint8_t tile_id =
            read_memory_8(bus, oam + 2);

        uint8_t attr =
            read_memory_8(bus, oam + 3);

        int sprite_y =
            (int)raw_y - 16;

        int sprite_x =
            (int)raw_x - 8;

        /*
         * Does this sprite intersect the current line?
         */
        if ((int)ppu->ly < sprite_y ||
            (int)ppu->ly >= sprite_y + sprite_height) {
            continue;
        }

        sprites_found++;

        if (sprites_found > 10)
            break;

        int row =
            (int)ppu->ly - sprite_y;

        /*
         * Y flip
         */
        if (attr & (1 << 6)) {
            row =
                sprite_height - 1 - row;
        }

        /*
         * 8x16 sprites ignore tile ID bit 0.
         */
        if (sprite_height == 16) {

            tile_id &= 0xFE;

            if (row >= 8) {
                tile_id++;
                row -= 8;
            }
        }

        uint16_t tile_addr =
            0x8000 +
            ((uint16_t)tile_id * 16) +
            ((uint16_t)row * 2);

        uint8_t low =
            read_memory_8(bus, tile_addr);

        uint8_t high =
            read_memory_8(bus, tile_addr + 1);

        for (int pixel = 0; pixel < 8; pixel++) {

            int screen_x =
                sprite_x + pixel;

            if (screen_x < 0 ||
                screen_x >= 160) {
                continue;
            }

            /*
             * Normally:
             *
             * pixel 0 uses bit 7
             * pixel 7 uses bit 0
             */
            uint8_t bit;

            if (attr & (1 << 5)) {
                /* X flipped */
                bit = pixel;
            }
            else {
                bit = 7 - pixel;
            }

            uint8_t color_id =
                (((high >> bit) & 1) << 1) |
                 ((low  >> bit) & 1);

            /*
             * Sprite color 0 is transparent.
             */
            if (color_id == 0)
                continue;

            /*
             * Attribute bit 7:
             *
             * 0 -> sprite is above BG
             * 1 -> sprite is behind BG colors 1-3
             *
             * It still appears over BG color 0.
             */
            if ((attr & (1 << 7)) &&
                ppu->bg_color_id[screen_x] != 0) {
                continue;
            }

            /*
             * Attribute bit 4 selects sprite palette.
             */
            uint8_t palette =
                (attr & (1 << 4))
                ? ppu->obp1
                : ppu->obp0;

            uint8_t shade =
                (palette >> (color_id * 2)) & 0x03;

            ppu->framebuffer[
                (uint16_t)ppu->ly * 160 +
                screen_x
            ] = dmg_color(shade);
        }
    }
}