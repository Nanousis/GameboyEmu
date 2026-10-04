#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "bus.h"

typedef struct CPU {
    uint16_t AF;
    uint16_t BC;
    uint16_t DE;
    uint16_t HL;
    uint16_t PC; // Program Counter
    uint16_t SP; // Stack Pointer
    uint64_t cycles; // Cycle count
    uint8_t ime;
}CPU;

void decode(CPU *cpu, uint8_t opcode, Bus *bus);
uint8_t fetch(CPU *cpu, uint8_t *memory);
uint8_t decode_cb(CPU *cpu, uint8_t opcode, Bus *bus);
void tick(CPU *cpu, Bus *bus);
void reset(CPU *cpu);
void print_registers(CPU *cpu);
void push_stack(CPU *cpu, uint16_t value, Bus *bus);
uint16_t pop_stack(CPU *cpu, Bus *bus);
void not_implemented(const char *instruction);