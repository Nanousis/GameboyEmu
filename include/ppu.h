#pragma once
#include <stdint.h>
typedef struct Bus Bus;

typedef enum {
    PPU_MODE_HBLANK = 0,
    PPU_MODE_VBLANK = 1,
    PPU_MODE_OAM    = 2,
    PPU_MODE_DRAW   = 3
} PPUMode;

typedef struct {
    uint8_t lcdc;
    uint8_t stat;

    uint8_t scy;
    uint8_t scx;

    uint8_t ly;
    uint8_t lyc;

    uint8_t bgp;
    uint8_t obp0;
    uint8_t obp1;

    uint8_t wy;
    uint8_t wx;

    uint16_t dot;
    PPUMode mode;
    uint8_t render_frame;

    uint32_t framebuffer[160 * 144];
    uint8_t bg_color_id[160];
    uint8_t sprite_color_id[160];
} PPU;


void ppu_tick(PPU *ppu, Bus *bus, uint32_t cycles);
uint32_t dmg_color(uint8_t shade);

void ppu_event(PPU *ppu, Bus *bus);
void update_stat(Bus *bus);
void render_scanline(PPU *ppu, Bus *bus);
void render_background_line(PPU *ppu, Bus *bus);
void render_window_line(PPU *ppu, Bus *bus);
void render_sprite_line(PPU *ppu, Bus *bus);