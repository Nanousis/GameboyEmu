#include <stdio.h>
#include <stdint.h>
#include "bus.h"


uint8_t read_memory_8(Bus *bus, uint16_t address)
{
    switch (address) {
        case 0xFF00: 
            uint8_t result = 0xCF;

            /*
            * Preserve selection bits.
            */
            if (bus->joypad.select_dpad)
                result &= ~(1 << 4);

            if (bus->joypad.select_buttons)
                result &= ~(1 << 5);

            /*
            * D-pad selected.
            */
            if (bus->joypad.select_dpad) {
                if (bus->joypad.right)
                    result &= ~(1 << 0);

                if (bus->joypad.left)
                    result &= ~(1 << 1);

                if (bus->joypad.up)
                    result &= ~(1 << 2);

                if (bus->joypad.down)
                    result &= ~(1 << 3);
            }

            /*
            * Action buttons selected.
            */
            if (bus->joypad.select_buttons) {
                if (bus->joypad.a)
                    result &= ~(1 << 0);

                if (bus->joypad.b)
                    result &= ~(1 << 1);

                if (bus->joypad.select)
                    result &= ~(1 << 2);

                if (bus->joypad.start)
                    result &= ~(1 << 3);
            }

            return result;
        // Timer
        case 0xFF04: // DIV
            return bus->timer.div_counter >> 8;

        case 0xFF05: // TIMA
            return bus->timer.TIMA;

        case 0xFF06: // TMA
            return bus->timer.TMA;

        case 0xFF07: // TAC
            return bus->timer.TAC | 0xF8;


        // Interrupts
        case 0xFF0F: // IF
            return bus->IF;

        case 0xFFFF: // IE
            return bus->IE;


        // PPU
        case 0xFF40: // LCDC
            return bus->ppu.lcdc;

        case 0xFF41: // STAT
            return bus->ppu.stat | 0x80;

        case 0xFF42: // SCY
            return bus->ppu.scy;

        case 0xFF43: // SCX
            return bus->ppu.scx;

        case 0xFF44: // LY
            return bus->ppu.ly;

        case 0xFF45: // LYC
            return bus->ppu.lyc;

        case 0xFF47: // BGP
            return bus->ppu.bgp;

        case 0xFF48: // OBP0
            return bus->ppu.obp0;

        case 0xFF49: // OBP1
            return bus->ppu.obp1;

        case 0xFF4A: // WY
            return bus->ppu.wy;

        case 0xFF4B: // WX
            return bus->ppu.wx;

        default:
            return bus->memory[address];
    }
}

uint16_t read_memory_16(Bus *bus, uint16_t address)
{
    uint8_t lo = read_memory_8(bus, address);
    uint8_t hi = read_memory_8(bus, address + 1);

    return ((uint16_t)hi << 8) | lo;
}

void write_memory_8(Bus *bus, uint16_t address, uint8_t value)
{
    switch (address) {

        case 0xFF00:
            /*
            * Bit 4 low = select D-pad.
            * Bit 5 low = select buttons.
            */
            bus->joypad.select_dpad =
                !(value & (1 << 4));

            bus->joypad.select_buttons =
                !(value & (1 << 5));

            return;
        case 0xFF01:
            bus->memory[address] = value;
            return;

        case 0xFF02:
            bus->memory[address] = value;

            if (value == 0x81) {
                printf(COLOR_GREEN);
                putchar(bus->memory[0xFF01]);
                printf(COLOR_RESET);
                fflush(stdout);
            }
            return;

        // Timer
        case 0xFF04:
            bus->timer.div_counter = 0;
            return;

        case 0xFF05:
            bus->timer.TIMA = value;
            return;

        case 0xFF06:
            bus->timer.TMA = value;
            return;

        case 0xFF07:
            bus->timer.TAC = value & 0x07;
            return;


        // Interrupts
        case 0xFF0F:
            bus->IF = value;
            return;

        case 0xFFFF:
            bus->IE = value;
            return;


        // PPU
        case 0xFF40: // LCDC
            bus->ppu.lcdc = value;
            return;

        case 0xFF41: // STAT
            /*
             * CPU controls bits 3-6.
             * PPU controls bits 0-2.
             */
            bus->ppu.stat =
                (bus->ppu.stat & 0x07) |
                (value & 0x78);
            return;

        case 0xFF42: // SCY
            bus->ppu.scy = value;
            return;

        case 0xFF43: // SCX
            bus->ppu.scx = value;
            return;

        case 0xFF44: // LY
            /*
             * Read only.
             */
            return;

        case 0xFF45: // LYC
            bus->ppu.lyc = value;
            return;

        case 0xFF47: // BGP
            bus->ppu.bgp = value;
            return;

        case 0xFF48: // OBP0
            bus->ppu.obp0 = value;
            return;

        case 0xFF49: // OBP1
            bus->ppu.obp1 = value;
            return;

        case 0xFF4A: // WY
            bus->ppu.wy = value;
            return;

        case 0xFF4B: // WX
            bus->ppu.wx = value;
            return;


        default:
            bus->memory[address] = value;
            return;
    }
}
void write_memory_16(Bus *bus, uint16_t address, uint16_t value)
{
    write_memory_8(bus, address, value & 0xFF);
    write_memory_8(bus, address + 1, value >> 8);
}
void reset_bus(Bus *bus)
{
    bus->IF = 0xE1;
    bus->IE = 0x00;

    // Timer
    bus->timer.div_counter = 0;
    bus->timer.TIMA = 0;
    bus->timer.TMA = 0;
    bus->timer.TAC = 0;
    bus->timer.tima_counter = 0;
    // Joypad
    bus->joypad.right = 0;
    bus->joypad.left = 0;
    bus->joypad.up = 0;
    bus->joypad.down = 0;

    bus->joypad.a = 0;
    bus->joypad.b = 0;
    bus->joypad.select = 0;
    bus->joypad.start = 0;

    bus->joypad.select_dpad = 0;
    bus->joypad.select_buttons = 0;
    // PPU
    bus->ppu.lcdc = 0x91;

    bus->ppu.stat = 0x80 |
                    PPU_MODE_OAM;

    bus->ppu.scy = 0;
    bus->ppu.scx = 0;

    bus->ppu.ly = 0;
    bus->ppu.lyc = 0;

    bus->ppu.bgp = 0xFC;
    bus->ppu.obp0 = 0xFF;
    bus->ppu.obp1 = 0xFF;

    bus->ppu.wy = 0;
    bus->ppu.wx = 0;

    bus->ppu.dot = 0;
    bus->ppu.mode = PPU_MODE_OAM;

    bus->ppu.render_frame = 0;
}