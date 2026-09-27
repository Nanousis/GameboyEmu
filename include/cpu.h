#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct CPU {
    uint8_t A;
    uint8_t B;
    uint8_t C;
    uint8_t D;
    uint8_t E;
    uint8_t F;
    uint8_t H;
    uint8_t L;
    uint16_t PC; // Program Counter
    uint16_t SP; // Stack Pointer
}CPU;

void decode(CPU *cpu, uint8_t opcode);
uint8_t fetch(CPU *cpu, uint8_t *memory);
void tick(CPU *cpu, uint8_t *memory);
void reset(CPU *cpu);