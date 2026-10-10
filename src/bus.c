#include <stdio.h>
#include <stdint.h>
#include "bus.h"


uint8_t read_memory_8(Bus *bus, uint16_t address)
{
    switch (address) {
        case 0xFF04:
            return bus->timer.div_counter >> 8;

        case 0xFF05:
            return bus->timer.TIMA;

        case 0xFF06:
            return bus->timer.TMA;

        case 0xFF07:
            return bus->timer.TAC | 0xF8;
        case 0xFF0F:      // IF - Interrupt flags
            return bus->IF;
        case 0xFFFF:      // IE - Interrupt enable
            return bus->IE;
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
        case 0xFF01:      // SB - Serial data
            bus->memory[address] = value;
            break;

        case 0xFF02:      // SC - Serial control
            bus->memory[address] = value;

            /*
             * Blargg test ROM convention:
             * FF01 = character
             * FF02 = 0x81 => output character
             */
            if (value == 0x81) {
                printf(COLOR_GREEN);
                putchar(bus->memory[0xFF01]);
                printf(COLOR_RESET);
                fflush(stdout);
            }
            break;
        case 0xFF04: // DIV
            bus->timer.div_counter = 0;
            return;

        case 0xFF05: // TIMA
            bus->timer.TIMA = value;
            return;

        case 0xFF06: // TMA
            bus->timer.TMA = value;
            return;

        case 0xFF07: // TAC
            bus->timer.TAC = value & 0x07;
            return;

        case 0xFF0F:      // IF - Interrupt flags
            bus->IF = value;
            break;

        case 0xFFFF:      // IE - Interrupt enable
            bus->IE = value;
            break;

        default:
            bus->memory[address] = value;
            break;
    }
}

void write_memory_16(Bus *bus, uint16_t address, uint16_t value)
{
    write_memory_8(bus, address, value & 0xFF);
    write_memory_8(bus, address + 1, value >> 8);
}
void reset_bus(Bus *bus) {
    bus->IF = 0xE1; // Reset IF register to its default value
    bus->IE = 0x00; // Reset IE register to its default value
}