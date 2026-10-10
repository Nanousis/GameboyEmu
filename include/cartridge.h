#pragma once

#include <stdint.h>
#include <stddef.h>

typedef enum {
    CART_ROM_ONLY,
    CART_MBC1,
    CART_UNSUPPORTED
} CartridgeController;

typedef struct {
    uint8_t *rom;
    size_t rom_size;

    uint8_t *ram;
    size_t ram_size;

    CartridgeController controller;

    /*
     * Raw MBC1 registers.
     */
    uint8_t ram_enabled;

    uint8_t rom_bank_low5;
    uint8_t bank_high2;

    /*
     * 0 = ROM banking mode
     * 1 = RAM banking / advanced mode
     */
    uint8_t banking_mode;

    uint8_t has_battery;
} Cartridge;


int cartridge_init(
    Cartridge *cart,
    uint8_t *rom,
    size_t rom_size
);

void cartridge_destroy(Cartridge *cart);

uint8_t cartridge_read(Cartridge *cart, uint16_t address);

void cartridge_write(Cartridge *cart, uint16_t address, uint8_t value);