#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "timer.h"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_RESET   "\x1b[0m"

typedef struct Bus {
    uint8_t *memory;
    Timer timer;
    uint8_t IF;
    uint8_t IE;
} Bus;

uint16_t read_memory_16(Bus *bus, uint16_t address);
uint8_t read_memory_8(Bus *bus, uint16_t address);
void write_memory_8(Bus *bus, uint16_t address, uint8_t value);
void write_memory_16(Bus *bus, uint16_t address, uint16_t value);
void reset_bus(Bus *bus);