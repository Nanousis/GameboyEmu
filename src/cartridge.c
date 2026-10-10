#include "cartridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static size_t ram_size_from_header(uint8_t code)
{
    switch (code) {
        case 0x00:
            return 0;

        case 0x01:
            return 2 * 1024;

        case 0x02:
            return 8 * 1024;

        case 0x03:
            return 32 * 1024;

        case 0x04:
            return 128 * 1024;

        case 0x05:
            return 64 * 1024;

        default:
            return 0;
    }
}


int cartridge_init(
    Cartridge *cart,
    uint8_t *rom,
    size_t rom_size
)
{
    memset(cart, 0, sizeof(*cart));

    if (!rom || rom_size < 0x150) {
        fprintf(stderr, "Invalid ROM\n");
        return 0;
    }

    cart->rom = rom;
    cart->rom_size = rom_size;

    uint8_t cartridge_type = rom[0x0147];
    uint8_t ram_size_code  = rom[0x0149];

    switch (cartridge_type) {

        case 0x00:
            cart->controller = CART_ROM_ONLY;
            break;

        case 0x01:
            cart->controller = CART_MBC1;
            break;

        case 0x02:
            cart->controller = CART_MBC1;
            break;

        case 0x03:
            cart->controller = CART_MBC1;
            cart->has_battery = 1;
            break;
        case 0x11: // MBC3
        case 0x12: // MBC3 + RAM
        case 0x13: // MBC3 + RAM + BATTERY
            cart->controller = CART_MBC3;
            cart->has_battery = (cartridge_type == 0x13);
            break;
        default:
            fprintf(
                stderr,
                "Unsupported cartridge type: 0x%02X\n",
                cartridge_type
            );

            cart->controller = CART_UNSUPPORTED;
            return 0;
    }
    cart->mbc3_rom_bank = 1;
    cart->mbc3_ram_bank = 0;
    cart->ram_size =
        ram_size_from_header(ram_size_code);

    if (cart->ram_size != 0) {
        cart->ram = calloc(cart->ram_size, 1);

        if (!cart->ram) {
            fprintf(stderr, "Failed to allocate cartridge RAM\n");
            return 0;
        }
    }

    /*
     * MBC1 power-on state.
     *
     * Switchable ROM area initially uses bank 1.
     */
    cart->rom_bank_low5 = 1;
    cart->bank_high2 = 0;
    cart->banking_mode = 0;
    cart->ram_enabled = 0;
    printf("Cartridge type: 0x%02X\n", cartridge_type);
    printf("ROM size: %zu bytes\n", rom_size);
    printf("RAM size: %zu bytes\n", cart->ram_size);
    return 1;
}


void cartridge_destroy(Cartridge *cart)
{
    free(cart->ram);
    cart->ram = NULL;

    /*
     * If Cartridge owns the ROM allocation:
     */
    free(cart->rom);
    cart->rom = NULL;

    cart->rom_size = 0;
    cart->ram_size = 0;
}
static size_t cartridge_rom_bank_count(
    const Cartridge *cart
)
{
    return cart->rom_size / 0x4000;
}
static uint8_t mbc1_read_rom0(
    Cartridge *cart,
    uint16_t address
)
{
    size_t bank = 0;

    if (cart->banking_mode == 1) {
        bank =
            ((size_t)cart->bank_high2 << 5);
    }

    size_t bank_count =
        cartridge_rom_bank_count(cart);

    if (bank_count != 0)
        bank %= bank_count;

    size_t offset =
        bank * 0x4000 +
        address;

    if (offset >= cart->rom_size)
        return 0xFF;

    return cart->rom[offset];
}
static uint8_t mbc1_read_romx(
    Cartridge *cart,
    uint16_t address
)
{
    uint8_t low = cart->rom_bank_low5 & 0x1F;

    if (low == 0)
        low = 1;

    size_t bank =
        ((size_t)(cart->bank_high2 & 0x03) << 5) |
        low;

    size_t bank_count =
        cartridge_rom_bank_count(cart);

    if (bank_count != 0)
        bank %= bank_count;

    size_t offset =
        bank * 0x4000 +
        (address - 0x4000);

    if (offset >= cart->rom_size)
        return 0xFF;

    return cart->rom[offset];
}
static uint8_t mbc1_read_ram(
    Cartridge *cart,
    uint16_t address
)
{
    if (!cart->ram_enabled)
        return 0xFF;

    if (!cart->ram || cart->ram_size == 0)
        return 0xFF;

    size_t bank = 0;

    if (cart->banking_mode == 1) {
        bank = cart->bank_high2 & 0x03;
    }

    size_t offset =
        bank * 0x2000 +
        (address - 0xA000);

    if (offset >= cart->ram_size)
        return 0xFF;

    return cart->ram[offset];
}
static void mbc1_write_ram(
    Cartridge *cart,
    uint16_t address,
    uint8_t value
)
{
    if (!cart->ram_enabled)
        return;

    if (!cart->ram || cart->ram_size == 0)
        return;

    size_t bank = 0;

    if (cart->banking_mode == 1) {
        bank = cart->bank_high2 & 0x03;
    }

    size_t offset =
        bank * 0x2000 +
        (address - 0xA000);

    if (offset >= cart->ram_size)
        return;

    cart->ram[offset] = value;
}
static uint8_t mbc3_read(Cartridge *cart, uint16_t address)
{
    // Fixed ROM bank 0
    if (address < 0x4000) {
        return address < cart->rom_size
            ? cart->rom[address]
            : 0xFF;
    }

    // Switchable ROM banks
    if (address < 0x8000) {
        size_t bank = cart->mbc3_rom_bank & 0x7F;

        if (bank == 0)
            bank = 1;

        size_t offset =
            bank * 0x4000 + (address - 0x4000);

        return offset < cart->rom_size
            ? cart->rom[offset]
            : 0xFF;
    }

    // External cartridge RAM
    if (address >= 0xA000 && address <= 0xBFFF) {
        if (!cart->ram_enabled || !cart->ram)
            return 0xFF;

        if (cart->mbc3_ram_bank > 3)
            return 0xFF;

        size_t offset =
            (size_t)cart->mbc3_ram_bank * 0x2000 +
            (address - 0xA000);

        return offset < cart->ram_size
            ? cart->ram[offset]
            : 0xFF;
    }

    return 0xFF;
}
static void mbc3_write(
    Cartridge *cart,
    uint16_t address,
    uint8_t value
) {
    // RAM enable
    if (address < 0x2000) {
        cart->ram_enabled =
            (value & 0x0F) == 0x0A;
        return;
    }

    // Select ROM bank
    if (address < 0x4000) {
        cart->mbc3_rom_bank = value & 0x7F;

        if (cart->mbc3_rom_bank == 0)
            cart->mbc3_rom_bank = 1;

        return;
    }

    // Select RAM bank or RTC register
    if (address < 0x6000) {
        cart->mbc3_ram_bank = value;
        return;
    }

    // RTC latch: not needed for Pokemon Red
    if (address < 0x8000) {
        return;
    }

    // External RAM write
    if (address >= 0xA000 && address <= 0xBFFF) {
        if (!cart->ram_enabled || !cart->ram)
            return;

        if (cart->mbc3_ram_bank > 3)
            return;

        size_t offset =
            (size_t)cart->mbc3_ram_bank * 0x2000 +
            (address - 0xA000);

        if (offset < cart->ram_size)
            cart->ram[offset] = value;
    }
}
uint8_t cartridge_read(
    Cartridge *cart,
    uint16_t address
)
{
    switch (cart->controller) {

        case CART_ROM_ONLY:

            if (address < 0x8000) {
                if (address < cart->rom_size)
                    return cart->rom[address];

                return 0xFF;
            }

            if (address >= 0xA000 &&
                address <= 0xBFFF) {

                size_t offset =
                    address - 0xA000;

                if (cart->ram &&
                    offset < cart->ram_size) {
                    return cart->ram[offset];
                }

                return 0xFF;
            }

            break;


        case CART_MBC1:

            if (address <= 0x3FFF)
                return mbc1_read_rom0(
                    cart,
                    address
                );

            if (address <= 0x7FFF)
                return mbc1_read_romx(
                    cart,
                    address
                );

            if (address >= 0xA000 &&
                address <= 0xBFFF) {
                return mbc1_read_ram(
                    cart,
                    address
                );
            }

            break;

        case CART_MBC3:
            return mbc3_read(cart, address);
        default:
            break;
    }

    return 0xFF;
}
void cartridge_write(
    Cartridge *cart,
    uint16_t address,
    uint8_t value
)
{
    switch (cart->controller) {

        case CART_ROM_ONLY:

            if (address >= 0xA000 &&
                address <= 0xBFFF) {

                size_t offset =
                    address - 0xA000;

                if (cart->ram &&
                    offset < cart->ram_size) {
                    cart->ram[offset] = value;
                }
            }

            /*
             * Writes to ROM are ignored.
             */
            return;


        case CART_MBC1:

            /*
             * 0000-1FFF
             *
             * RAM enabled when low nibble == 0xA.
             */
            if (address <= 0x1FFF) {

                cart->ram_enabled =
                    ((value & 0x0F) == 0x0A);

                return;
            }

            /*
             * 2000-3FFF
             *
             * Lower 5 ROM bank bits.
             */
            if (address <= 0x3FFF) {

                cart->rom_bank_low5 =
                    value & 0x1F;

                /*
                 * 0 behaves like 1.
                 */
                if (cart->rom_bank_low5 == 0)
                    cart->rom_bank_low5 = 1;

                return;
            }

            /*
             * 4000-5FFF
             *
             * Upper two bank bits.
             */
            if (address <= 0x5FFF) {

                cart->bank_high2 =
                    value & 0x03;

                return;
            }

            /*
             * 6000-7FFF
             *
             * Banking mode.
             */
            if (address <= 0x7FFF) {

                cart->banking_mode =
                    value & 0x01;

                return;
            }

            /*
             * External RAM.
             */
            if (address >= 0xA000 &&
                address <= 0xBFFF) {

                mbc1_write_ram(
                    cart,
                    address,
                    value
                );

                return;
            }

            return;
        case CART_MBC3:
            mbc3_write(cart, address, value);
            return;

        default:
            return;
    }
}
int cartridge_save(Cartridge *cart, const char *path)
{
    if (!cart->has_battery || !cart->ram)
        return 0;

    FILE *file = fopen(path, "wb");

    if (!file)
        return 0;

    size_t written = fwrite(
        cart->ram,
        1,
        cart->ram_size,
        file
    );

    int closed = fclose(file);

    return written == cart->ram_size && closed == 0;
}
int cartridge_load_save(Cartridge *cart, const char *path)
{
    if (!cart->has_battery || !cart->ram)
        return 0;

    FILE *file = fopen(path, "rb");

    if (!file)
        return 0;

    size_t read = fread(
        cart->ram,
        1,
        cart->ram_size,
        file
    );

    int closed = fclose(file);

    return read == cart->ram_size && closed == 0;
}