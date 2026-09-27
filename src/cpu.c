#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "cpu.h"
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
void tick(CPU *cpu, uint8_t *memory) {

    uint8_t opcode = fetch(cpu, memory);
    decode(cpu, opcode);
}

void decode(CPU *cpu, uint8_t opcode) {
    // Decode the opcode and execute the corresponding instruction
    // For now, we will just print the opcode
    // printf("Opcode: 0x%02X\n", opcode);
    uint8_t block = opcode>>6;
    uint16_t newPC = cpu->PC + 1; // Increment PC by 1 for the next instruction
    printf("0x%X ", cpu->PC);
    // printf("0x%X: Opcode: 0x%02X: ", cpu->PC, opcode);
    switch(block) {
        case 0b00:
            switch(opcode & 0b111111) {
                case 0b000000: // NOP
                    printf("NOP\n");
                    break;
                case 0b000001: // LD r16, imm16
                case 0b010001: 
                case 0b100001: 
                case 0b110001: 
                    printf("LD r16, imm16\n");
                    newPC = cpu->PC + 3; // Increment PC by 3 for the next instruction (1 for opcode + 2 for imm16)
                    break;
                case 0b000010: // LD [r16], A
                case 0b010010:
                case 0b100010:
                case 0b110010:
                    printf("LD [r16], A\n");
                    break;
                case 0b001010: // LD A, [r16]
                case 0b011010:
                case 0b101010:
                case 0b111010:
                    printf("LD A, [r16]\n");
                    break;
                case 0b001000: // LD [imm16], A
                    printf("LD [imm16], A\n");
                    newPC = cpu->PC + 3; // Increment PC by 3 for the next instruction (1 for opcode + 2 for imm16)
                    break;
                case 0b000011: // INC   r16
                case 0b010011:
                case 0b100011:
                case 0b110011:
                    printf("INC   r16\n");
                    break;
                case 0b001011: // DEC r16
                case 0b011011:
                case 0b101011:
                case 0b111011:
                    printf("DEC r16\n");
                    break;
                case 0b001001: // add hl, r16
                case 0b011001:
                case 0b101001:
                case 0b111001:
                    printf("add hl, r16\n");
                    break;
                case 0b000100: // INC   r8
                case 0b001100:
                case 0b010100:
                case 0b011100:
                case 0b100100:
                case 0b101100:
                case 0b110100:
                case 0b111100:
                    printf("INC   r8\n");
                    break;
                case 0b000101: // DEC r8
                case 0b001101:
                case 0b010101:
                case 0b011101:
                case 0b100101:
                case 0b101101:
                case 0b110101:
                case 0b111101:
                    printf("DEC r8\n");
                    break;
                case 0b000110: // LD r8, imm8
                case 0b001110:
                case 0b010110:
                case 0b011110:
                case 0b100110:
                case 0b101110:
                case 0b110110:
                case 0b111110:
                    printf("LD r8, imm8\n");
                    newPC = cpu->PC + 2; // Increment PC by 2 for the next instruction (1 for opcode + 1 for imm8)
                    break;
                case 0b000111: // rlca
                    printf("rlca\n");
                    break;
                case 0b001111: // rrca
                    printf("rrca\n");
                    break;
                case 0b010111: // rla
                    printf("rla\n");
                    break;
                case 0b011111: // rra
                    printf("rra\n");
                    break;
                case 0b100111: // daa
                    printf("daa\n");
                    break;
                case 0b101111: // cpl
                    printf("cpl\n");
                    break;
                case 0b110111: // scf
                    printf("scf\n");
                    break;
                case 0b111111: // ccf
                    printf("ccf\n");
                    break;

                case 0b011000: // JR imm8
                    printf("JR imm8\n");
                    newPC = cpu->PC + 2;
                    break;
                case 0b100000: // JR cond, imm8
                case 0b101000:
                case 0b110000:
                case 0b111000:
                    printf("JR cond, imm8\n");
                    newPC = cpu->PC + 2;
                    break;
                case 0b010000: // stop:
                    printf("stop\n");
                    newPC = cpu->PC + 2;
                    break;
                default:
                    printf("Unknown instruction in block 0\n");
            }
            break;
        case 0b01:
            switch(opcode & 0b111111) {
                case 0b01110110:
                    printf("HALT instruction encountered\n");
                    break;
                default:
                    printf("LD r8, r8\n");
            }
            break;
        case 0b10: // BLOCK 2 instructions
            uint8_t sub_block = (opcode >> 3)&0b111;
            uint8_t reg = opcode & 0b111;
            // printf("Sub-block: 0x%02X, Register: 0x%02X\n", sub_block, reg);
            switch(sub_block) {
                case 0b000: // add a, r8
                    printf("add a, r8\n");
                    break;
                case 0b001: //  adc a, r8
                    printf("adc a, r8\n");
                    break;
                case 0b010: // sub a, r8
                    printf("sub a, r8\n");
                    break;
                case 0b011: // subc a, r8
                    printf("subc a, r8\n");
                    break;
                case 0b100: // and a, r8
                    printf("and a, r8\n");
                    break;
                case 0b101: // xor a, r8
                    printf("xor a, r8\n");
                    break;
                case 0b110: // or a, r8
                    printf("or a, r8\n");
                    break;
                case 0b111: // cp a, r8
                    printf("cp a, r8\n");
                    break;
                default:
                    printf("Unknown sub-block in block 2\n");
            }
            break;
        case 0b11: // Block 3 instructions
            switch(opcode & 0b111111) {
                ////// first part of the block 3 instructions -> ALU
                case 0b000110: // add a imm 8
                    printf("add a imm 8\n");
                    newPC = cpu->PC + 2;
                    break;
                case 0b001110: // addC a imm 8
                    printf("addC a imm 8\n");
                    newPC = cpu->PC + 2;
                    break;
                case 0b010110: // sub a imm 8
                    printf("sub a imm 8\n");
                    newPC = cpu->PC + 2;
                    break;
                case 0b011110: // subC a imm 8
                    printf("subC a imm 8\n");
                    newPC = cpu->PC + 2;
                    break;
                case 0b100110: // and a imm 8
                    printf("and a imm 8\n");
                    newPC = cpu->PC + 2;
                    break;
                case 0b101110: // xor a imm 8
                    printf("xor a imm 8\n");
                    newPC = cpu->PC + 2;
                    break;
                case 0b110110: // or a imm 8
                    printf("or a imm 8\n");
                    newPC = cpu->PC + 2;
                    break;
                case 0b111110: // cp a imm 8
                    printf("cp a imm 8\n");
                    newPC = cpu->PC + 2;
                    break;

                ////// second part of the block 3 instructions -> control flow
                case 0b000000: // ret cond
                case 0b001000: // ret cond
                case 0b010000: // ret cond
                case 0b011000: // ret cond
                    printf("ret cond\n");
                    break;
                case 0b000010: // JP cond, imm16
                case 0b001010:
                case 0b010010:
                case 0b011010:
                    printf("JP cond, imm16\n");
                    newPC = cpu->PC + 3;
                    break;
                case 0b000011: // JP im16
                    printf("jp im16\n");
                    newPC = cpu->PC + 3;
                    break;
                case 0b101001: // Jp HL
                    printf("jp hl\n");
                    break;
                case 0b000100: // Call cond, imm16
                case 0b001100:
                case 0b010100:
                case 0b011100:
                    printf("Call cond, imm16\n");
                    newPC = cpu->PC + 3;
                    break;
                case 0b001101:
                    printf("call im16\n");
                    newPC = cpu->PC + 3;
                    break;
                case 0b000111: // rst tgt3
                case 0b001111:
                case 0b010111:
                case 0b011111:
                case 0b100111:
                case 0b101111:
                case 0b110111:
                case 0b111111:
                    printf("rst tgt3\n");
                    break;

                ////// third part of the block 3 instructions -> misc
                case 0b000001: // pop rp2
                case 0b010001: // pop rp2
                case 0b100001: // pop rp2
                case 0b110001: // pop rp2
                    printf("pop rp2\n");
                    break;
                case 0b000101: // push rp2
                case 0b010101: // push rp2
                case 0b100101: // push rp2
                case 0b110101: // push rp2
                    printf("push rp2\n");
                    break;
                /// fourth part of the block 3 instructions -> loads
                case 0b100010: // ldh [c], a
                    printf("LD [c], a\n");
                    newPC = cpu->PC + 2;
                    break;
                case 0b100000: // ldh [imm8], a
                    printf("LD [imm8], a\n");
                    newPC = cpu->PC + 2;
                    break;
                case 0b101010: // LD [imm16], a
                    printf("LD [imm16], a\n");
                    newPC = cpu->PC + 3;
                    break;
                case 0b110010: // ldh a, [c]
                    printf("LD a, [c]\n");
                    newPC = cpu->PC + 2;
                    break;
                case 0b110000: // ldh a, [imm8]
                    printf("ldh a, [imm8]\n");
                    newPC = cpu->PC + 2;
                    break;
                case 0b111010: // LD a, [imm16]
                    printf("LD a, [imm16]\n");
                    newPC = cpu->PC + 3;
                    break;

                // fifth part of the block 3 instructions -> misc
                case 0b101000: // add sp,im8
                    printf("add sp, im8\n");
                    newPC = cpu->PC + 2;
                    break;
                case 0b111000: // LD hl, sp+im8
                    printf("LD hl, sp+im8\n");
                    newPC = cpu->PC + 2;
                    break;
                case 0b111001: // LD sp, hl
                    printf("LD sp, hl\n");
                    break;
                
                // sixth part of the block 3 instructions -> ei / di
                case 0b110011: // di
                    printf("di\n");
                    break;
                case 0b111011: // ei
                    printf("ei\n");
                    break;
                case (0xCB&0b111111): // CB prefix
                    printf("CB prefix\n");
                    newPC = cpu->PC + 2;
                    break;
                default:
                    printf("Unknown instruction in block 3\n");
                    break;
            }
            break;
    }
    cpu->PC = newPC;
}

uint8_t fetch(CPU *cpu, uint8_t *memory) {
    // Fetch the next instruction from memory at the program counter (PC)
    // For now, we will just INC  rement the PC to simulate fetching an instruction
    uint8_t instruction = memory[cpu->PC];
    return instruction;
}

void reset(CPU *cpu){
    cpu->A = 0;
    cpu->B = 0;
    cpu->C = 0;
    cpu->D = 0;
    cpu->E = 0;
    cpu->F = 0;
    cpu->H = 0;
    cpu->L = 0;
    cpu->PC = 0x150; // Start execution at address 0x100
    cpu->SP = 0xFFFE; // Initialize stack pointer
}