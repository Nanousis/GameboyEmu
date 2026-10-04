#include <stdio.h>
#include <stdint.h>
#include "bus.h"

uint8_t read_memory_8(Bus *bus, uint16_t address)
{
    return bus->memory[address];
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

        case 0xFF0F:      // IF - Interrupt flags
            bus->memory[address] = value;
            break;

        case 0xFFFF:      // IE - Interrupt enable
            bus->memory[address] = value;
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