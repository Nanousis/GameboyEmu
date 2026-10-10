#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "cpu.h"
#include "bus.h"
#include "timer.h"

// #define DEBUG_PRINTS

#ifdef DEBUG_PRINTS
    #define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...) ((void)0)
#endif

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define set_bit(value, bit) ((value) |= (1U << (bit)))
#define clear_bit(value, bit) ((value) &= ~(1U << (bit)))
#define check_bit(value, bit) (((value) >> (bit)) & 1U)
void tick(CPU *cpu, Bus *bus) {

    uint8_t cycles_taken;
    uint8_t pending_interrupts = get_pending_interrupts(bus);
    if (pending_interrupts) {

        cpu->halted = 0; // Wake up from HALT if any interrupt is pending
        if(service_interrupt(cpu, bus, pending_interrupts)) {
            cycles_taken = 20; // Interrupt entry costs 20 T-cycles
            goto finish_tick;
        }
    }
    if (cpu->halted) {
        cycles_taken = 4;
        goto finish_tick;
    }
    uint8_t opcode = fetch(cpu, bus->memory);
    cycles_taken = decode(cpu, opcode, bus);
    finish_tick:
    cpu->cycles += cycles_taken;
    timer_tick(bus, cycles_taken);
    if (cpu->ime_enable_delay > 0) {
        cpu->ime_enable_delay--;

        if (cpu->ime_enable_delay == 0) {
            cpu->ime = 1;
        }
    }

    
}

uint8_t decode(CPU *cpu, uint8_t opcode, Bus *bus) {
    // Decode the opcode and execute the corresponding instruction
    // For now, we will just print the opcode
    // DEBUG_PRINTF("Opcode: 0x%02X\n", opcode);
    uint8_t block = opcode>>6;
    uint16_t newPC = cpu->PC + 1; // Increment PC by 1 for the next instruction
    DEBUG_PRINTF("0x%X ", cpu->PC);
    int8_t offset;
    uint8_t a;
    uint8_t taken = 0;
    uint8_t result_8;
    uint16_t result;
    uint32_t result_32;
    uint8_t cycles_taken = 4;
    // DEBUG_PRINTF("0x%X: Opcode: 0x%02X: ", cpu->PC, opcode);
    switch(block) {
        case 0b00:
            switch(opcode & 0b111111) {
                case 0b000000: // NOP
                    DEBUG_PRINTF("NOP\n");
                    break;
                case 0b000001: // LD r16, imm16
                case 0b010001: 
                case 0b100001: 
                case 0b110001: 
                    cycles_taken = 12; // Increment cycle count by 8 for this instruction
                    // DEBUG_PRINTF("LD r16, imm16\n");
                    newPC = cpu->PC + 3; // Increment PC by 3 for the next instruction (1 for opcode + 2 for imm16)
                    switch(opcode>>4 & 0b11) {
                        case 0b00: // BC
                            cpu->BC = read_memory_16(bus, cpu->PC + 1);
                            break;
                        case 0b01: // DE
                            cpu->DE = read_memory_16(bus, cpu->PC + 1);
                            break;
                        case 0b10: // HL
                            cpu->HL = read_memory_16(bus, cpu->PC + 1);
                            break;
                        case 0b11: // SP
                            cpu->SP = read_memory_16(bus, cpu->PC + 1);
                            break;
                    }
                    break;
                case 0b000010: // LD [r16], A
                case 0b010010:
                case 0b100010:
                case 0b110010:
                    cycles_taken = 8; // Increment cycle count by 8 for this instruction
                    switch(opcode>>4 & 0b11) {
                        case 0b00: // BC
                            write_memory_8(bus, cpu->BC, cpu->AF>>8);
                            break;
                        case 0b01: // DE
                            write_memory_8(bus, cpu->DE, cpu->AF>>8);
                            break;
                        case 0b10: // HL
                            write_memory_8(bus, cpu->HL++, cpu->AF>>8);
                            break;
                            case 0b11: // SP
                            write_memory_8(bus, cpu->HL--, cpu->AF>>8);
                            break;
                    }
                    break;
                case 0b001010: // LD A, [r16]
                case 0b011010:
                case 0b101010:
                case 0b111010:
                    switch(opcode>>4 & 0b11) {
                        cycles_taken = 8;
                        case 0b00: // BC
                            cpu->AF = (read_memory_8(bus, cpu->BC) << 8) | (cpu->AF & 0xFF);
                            break;
                        case 0b01: // DE
                            cpu->AF = (read_memory_8(bus, cpu->DE) << 8) | (cpu->AF & 0xFF);
                            break;
                        case 0b10: // HL
                            cpu->AF = (read_memory_8(bus, cpu->HL++) << 8) | (cpu->AF & 0xFF);
                            break;
                        case 0b11: // SP
                            cpu->AF = (read_memory_8(bus, cpu->HL--) << 8) | (cpu->AF & 0xFF);
                            break;
                    }
                    cycles_taken = 8; // Increment cycle count by 8 for this instruction
                    break;
                case 0b001000: // LD [imm16], SP
                    write_memory_16(bus, read_memory_16(bus, cpu->PC + 1), cpu->SP);
                    newPC = cpu->PC + 3; // Increment PC by 3 for the next instruction (1 for opcode + 2 for imm16)
                    cycles_taken = 20;
                    break;
                case 0b000011: // INC   r16
                case 0b010011:
                case 0b100011:
                case 0b110011:
                    switch(opcode>>4 & 0b11) {
                        case 0b00: // BC
                            result = ++cpu->BC;
                            break;
                        case 0b01: // DE
                            result = ++cpu->DE;
                            break;
                        case 0b10: // HL
                            result = ++cpu->HL;
                            break;
                        case 0b11: // SP
                            result = ++cpu->SP;
                            break;
                    }
                    cycles_taken = 8;
                    break;
                case 0b001011: // DEC r16
                case 0b011011:
                case 0b101011:
                case 0b111011:
                    switch(opcode>>4 & 0b11) {
                        case 0b00: // BC
                            result = --cpu->BC;
                            break;
                        case 0b01: // DE
                            result = --cpu->DE;
                            break;
                        case 0b10: // HL
                            result = --cpu->HL;
                            break;
                        case 0b11: // SP
                            result = --cpu->SP;
                            break;
                    }
                    cycles_taken = 8;
                    break;
                case 0b001001: // add hl, r16
                case 0b011001:
                case 0b101001:
                case 0b111001:
                    switch ((opcode >> 4) & 0b11) {
                        case 0b00: result = cpu->BC; break;
                        case 0b01: result = cpu->DE; break;
                        case 0b10: result = cpu->HL; break;
                        case 0b11: result = cpu->SP; break;
                    }

                    result_32 = (uint32_t)cpu->HL + result;

                    clear_bit(cpu->AF, 6);

                    if (((cpu->HL & 0x0FFF) + (result & 0x0FFF)) > 0x0FFF)
                        set_bit(cpu->AF, 5);
                    else
                        clear_bit(cpu->AF, 5);

                    if (result_32 > 0xFFFF)
                        set_bit(cpu->AF, 4);
                    else
                        clear_bit(cpu->AF, 4);

                    cpu->HL = result_32 & 0xFFFF;
                    cycles_taken = 8;
                    break;
                case 0b000100: // INC   r8
                case 0b001100:
                case 0b010100:
                case 0b011100:
                case 0b100100:
                case 0b101100:
                case 0b110100:
                case 0b111100:
                    switch(opcode>>3 & 0b111) {
                        case 0b000: // B
                            result_8 = (cpu->BC >> 8) + 1;
                            cpu->BC = (cpu->BC & 0x00FF) | (result_8 << 8);
                            break;
                        case 0b001: // C
                            result_8 = (cpu->BC & 0xFF) + 1;
                            cpu->BC = (cpu->BC & 0xFF00) | result_8;
                            break;
                        case 0b010: // D
                            result_8 = (cpu->DE >> 8) + 1;
                            cpu->DE = (cpu->DE & 0x00FF) | (result_8 << 8);
                            break;
                        case 0b011: // E
                            result_8 = (cpu->DE & 0xFF) + 1;
                            cpu->DE = (cpu->DE & 0xFF00) | result_8;
                            break;
                        case 0b100: // H
                            result_8 = (cpu->HL >> 8) + 1;
                            cpu->HL = (cpu->HL & 0x00FF) | (result_8 << 8);
                            break;
                        case 0b101: // L
                            result_8 = (cpu->HL & 0xFF) + 1;
                            cpu->HL = (cpu->HL & 0xFF00) | result_8;
                            break;
                        case 0b110: // [HL]
                            result_8 = read_memory_8(bus, cpu->HL) + 1;
                            write_memory_8(bus, cpu->HL, result_8);
                            cycles_taken = 12;
                            break;
                        case 0b111: // A
                            result_8 = (cpu->AF >> 8) + 1;
                            cpu->AF = (cpu->AF & 0x00FF) | ((uint16_t)result_8 << 8);
                            break;
                    }
                    if(result_8 == 0) {
                        // Set zero flag
                        set_bit(cpu->AF, 7);
                    } else {
                        // Clear zero flag
                        clear_bit(cpu->AF, 7);
                    }
                    clear_bit(cpu->AF, 6); // Clear subtract flag
                    if((result_8 & 0x0F) == 0) {
                        // Set half-carry flag
                        set_bit(cpu->AF, 5);
                    } else {
                        // Clear half-carry flag
                        clear_bit(cpu->AF, 5);
                    }
                    break;
                case 0b000101: // DEC r8
                case 0b001101:
                case 0b010101:
                case 0b011101:
                case 0b100101:
                case 0b101101:
                case 0b110101:
                case 0b111101:
                    switch(opcode>>3 & 0b111) {
                        case 0b000: // B
                            result_8 = (cpu->BC >> 8) - 1;
                            cpu->BC = (cpu->BC & 0x00FF) | (result_8 << 8);
                            break;
                        case 0b001: // C
                            result_8 = (cpu->BC & 0xFF) - 1;
                            cpu->BC = (cpu->BC & 0xFF00) | result_8;
                            break;
                        case 0b010: // D
                            result_8 = (cpu->DE >> 8) - 1;
                            cpu->DE = (cpu->DE & 0x00FF) | (result_8 << 8);
                            break;
                        case 0b011: // E
                            result_8 = (cpu->DE & 0xFF) - 1;
                            cpu->DE = (cpu->DE & 0xFF00) | result_8;
                            break;
                        case 0b100: // H
                            result_8 = (cpu->HL >> 8) - 1;
                            cpu->HL = (cpu->HL & 0x00FF) | (result_8 << 8);
                            break;
                        case 0b101: // L
                            result_8 = (cpu->HL & 0xFF) - 1;
                            cpu->HL = (cpu->HL & 0xFF00) | result_8;
                            break;
                        case 0b110: // [HL]
                            result_8 = read_memory_8(bus, cpu->HL) - 1;
                            write_memory_8(bus, cpu->HL, result_8);
                            cycles_taken = 12;
                            break;
                        case 0b111: // A
                            result_8 = (cpu->AF >> 8) - 1;
                            cpu->AF = (cpu->AF & 0x00FF) | ((uint16_t)result_8 << 8);
                            break;
                    }
                    if(result_8 == 0) {
                        // Set zero flag
                        set_bit(cpu->AF, 7);
                    } else {
                        // Clear zero flag
                        clear_bit(cpu->AF, 7);
                    }
                    set_bit(cpu->AF, 6); // Set subtract flag
                    if((result_8 & 0x0F) == 0x0F) {
                        // Set half-carry flag
                        set_bit(cpu->AF, 5);
                    } else {
                        // Clear half-carry flag
                        clear_bit(cpu->AF, 5);
                    }    
                    break;
                case 0b000110: // LD r8, imm8
                case 0b001110:
                case 0b010110:
                case 0b011110:
                case 0b100110:
                case 0b101110:
                case 0b110110:
                case 0b111110:
                    result = read_memory_8(bus, cpu->PC + 1);
                    cycles_taken = 8;
                    switch ((opcode >> 3) & 0b111){
                        case 0b000: // B
                            cpu->BC = (cpu->BC & 0x00FF) | ((uint16_t)result << 8);
                            break;
                        case 0b001: // C
                            cpu->BC = (cpu->BC & 0xFF00) | result;
                            break;
                        case 0b010: // D
                            cpu->DE = (cpu->DE & 0x00FF) | ((uint16_t)result << 8);
                            break;
                        case 0b011: // E
                            cpu->DE = (cpu->DE & 0xFF00) | result;
                            break;
                        case 0b100: // H
                            cpu->HL = (cpu->HL & 0x00FF) | ((uint16_t)result << 8);
                            break;
                        case 0b101: // L
                            cpu->HL = (cpu->HL & 0xFF00) | result;
                            break;
                        case 0b110: // [HL]
                            write_memory_8(bus, cpu->HL, result);
                            cycles_taken = 12;
                            break;
                        case 0b111: // A
                            cpu->AF = (cpu->AF & 0x00FF) | ((uint16_t)result << 8);
                            break;
                    }
                    
                    newPC = cpu->PC + 2; // Increment PC by 2 for the next instruction (1 for opcode + 1 for imm8)
                    break;
                case 0b000111:{ // rlca.
                    a = cpu->AF >> 8;
                    uint8_t carry = a >> 7;

                    a = (a << 1) | carry;

                    cpu->AF = ((uint16_t)a << 8) | (carry << 4);
                    break;
                }
                case 0b001111: { // RRCA
                    a = cpu->AF >> 8;
                    uint8_t carry = a & 0x01;

                    a = (a >> 1) | (carry << 7);

                    // Z=0, N=0, H=0, C=old bit 0
                    cpu->AF = ((uint16_t)a << 8) | (carry << 4);

                    DEBUG_PRINTF("rrca\n");
                    break;
                }

                case 0b010111: { // RLA
                    a = cpu->AF >> 8;
                    uint8_t old_carry = (cpu->AF >> 4) & 0x01;
                    uint8_t new_carry = (a >> 7) & 0x01;

                    a = (a << 1) | old_carry;

                    // Z=0, N=0, H=0, C=old bit 7
                    cpu->AF = ((uint16_t)a << 8) | (new_carry << 4);

                    DEBUG_PRINTF("rla\n");
                    break;
                }

                case 0b011111: { // RRA
                    a = cpu->AF >> 8;
                    uint8_t old_carry = (cpu->AF >> 4) & 0x01;
                    uint8_t new_carry = a & 0x01;

                    a = (a >> 1) | (old_carry << 7);

                    // Z=0, N=0, H=0, C=old bit 0
                    cpu->AF = ((uint16_t)a << 8) | (new_carry << 4);

                    DEBUG_PRINTF("rra\n");
                    break;
                }

                case 0b100111: { // DAA
                    a = cpu->AF >> 8;
                    uint8_t f = cpu->AF & 0xF0;

                    uint8_t n = (f >> 6) & 1;
                    uint8_t h = (f >> 5) & 1;
                    uint8_t c = (f >> 4) & 1;

                    uint8_t correction = 0;

                    if (!n) {
                        if (h || (a & 0x0F) > 0x09) {
                            correction |= 0x06;
                        }

                        if (c || a > 0x99) {
                            correction |= 0x60;
                            c = 1;
                        }

                        a += correction;
                    } else {
                        if (h) {
                            correction |= 0x06;
                        }

                        if (c) {
                            correction |= 0x60;
                        }

                        a -= correction;
                    }

                    // Z=result==0
                    // N=unchanged
                    // H=0
                    // C=updated/preserved
                    f = 0;

                    if (a == 0) {
                        f |= 1 << 7;
                    }

                    if (n) {
                        f |= 1 << 6;
                    }

                    if (c) {
                        f |= 1 << 4;
                    }

                    cpu->AF = ((uint16_t)a << 8) | f;

                    DEBUG_PRINTF("daa\n");
                    break;
                }

                case 0b101111: { // CPL
                    a = cpu->AF >> 8;
                    uint8_t f = cpu->AF & 0xF0;

                    a = ~a;

                    // Z unchanged
                    // C unchanged
                    // N=1
                    // H=1
                    f |= (1 << 6);
                    f |= (1 << 5);

                    cpu->AF = ((uint16_t)a << 8) | f;

                    DEBUG_PRINTF("cpl\n");
                    break;
                }

                case 0b110111: { // SCF
                    a = cpu->AF >> 8;
                    uint8_t f = cpu->AF & 0xF0;

                    // Z unchanged
                    // N=0
                    // H=0
                    // C=1
                    f &= ~(1 << 6);
                    f &= ~(1 << 5);
                    f |=  (1 << 4);

                    cpu->AF = ((uint16_t)a << 8) | f;

                    DEBUG_PRINTF("scf\n");
                    break;
                }

                case 0b111111: { // CCF
                    a = cpu->AF >> 8;
                    uint8_t f = cpu->AF & 0xF0;

                    // Z unchanged
                    // N=0
                    // H=0
                    // C=inverted
                    f &= ~(1 << 6);
                    f &= ~(1 << 5);
                    f ^=  (1 << 4);

                    cpu->AF = ((uint16_t)a << 8) | f;

                    DEBUG_PRINTF("ccf\n");
                    break;
                }

                case 0b011000: // JR imm8
                    DEBUG_PRINTF("JR imm8\n");
                    offset = (int8_t)read_memory_8(bus, cpu->PC + 1);
                    newPC = offset + cpu->PC + 2;
                    cycles_taken = 12;
                    break;
                case 0b100000: // JR cond, imm8
                    DEBUG_PRINTF("JR NZ, imm8\n");
                    taken = !check_bit(cpu->AF, 7); // Check if Z flag is not set
                    goto jr_cond;
                case 0b101000:
                    DEBUG_PRINTF("JR Z, imm8\n");
                    taken = check_bit(cpu->AF, 7); // Check if Z flag is set
                    goto jr_cond;
                case 0b110000:
                    DEBUG_PRINTF("JR NC, imm8\n");
                    taken = !check_bit(cpu->AF, 4); // Check if C flag is not set
                    goto jr_cond;
                case 0b111000:
                    DEBUG_PRINTF("JR C, imm8\n");
                    taken = check_bit(cpu->AF, 4); // Check if C flag is set
                    // goto jr_cond;
                    jr_cond:
                        offset = (int8_t)read_memory_8(bus, cpu->PC + 1);
                        newPC = cpu->PC + 2;
                        cycles_taken = 8; // Increment cycle count by 8 for this instruction
                        if(taken){
                            newPC += (offset & 0xFFFF); 
                            cycles_taken = 12; // Increment cycle count by 12 for this instruction
                        }
                        break;
                case 0b010000: // stop:
                    DEBUG_PRINTF("stop\n");
                    newPC = cpu->PC + 2;
                    not_implemented("STOP");
                    break;
                default:
                    DEBUG_PRINTF("Unknown instruction in block 0\n");
            }
            break;
        case 0b01:
            switch(opcode & 0b111111) {
                case 0b110110:
                    DEBUG_PRINTF("HALT instruction encountered\n");
                    cpu->halted = 1; // everyone keeps talking about the halt bug, shjould look into it
                    break;
                default:
                    switch(opcode&0b111) { // ld r8, r8
                        case 0b000: // B
                            DEBUG_PRINTF("LD r8, B\n");
                            result_8 = (cpu->BC >> 8);
                            break;
                        case 0b001: // C
                            DEBUG_PRINTF("LD r8, C\n");
                            result_8 = (cpu->BC & 0xFF);
                            break;
                        case 0b010: // D
                            DEBUG_PRINTF("LD r8, D\n");
                            result_8 = (cpu->DE >> 8);
                            break;
                        case 0b011: // E
                            DEBUG_PRINTF("LD r8, E\n");
                            result_8 = (cpu->DE & 0xFF);
                            break;
                        case 0b100: // H
                            DEBUG_PRINTF("LD r8, H\n");
                            result_8 = (cpu->HL >> 8);
                            break;
                        case 0b101: // L
                            DEBUG_PRINTF("LD r8, L\n");
                            result_8 = (cpu->HL & 0xFF);
                            break;
                        case 0b110: // [HL]
                            DEBUG_PRINTF("LD r8, [HL]\n");
                            cycles_taken += 4;
                            result_8 = read_memory_8(bus, cpu->HL);
                            break;
                        case 0b111: // A
                            DEBUG_PRINTF("LD r8, A\n");
                            result_8 = (cpu->AF >> 8);
                            break;
                        }
                    switch(opcode>>3&0b111) { // dest
                        case 0b000: // B
                            cpu->BC = (cpu->BC & 0x00FF) | ((uint16_t)result_8 << 8);
                            break;
                        case 0b001: // C
                            cpu->BC = (cpu->BC & 0xFF00) | result_8;
                            break;
                        case 0b010: // D
                            cpu->DE = (cpu->DE & 0x00FF) | ((uint16_t)result_8 << 8);
                            break;
                        case 0b011: // E
                            cpu->DE = (cpu->DE & 0xFF00) | result_8;
                            break;
                        case 0b100: // H
                            cpu->HL = (cpu->HL & 0x00FF) | ((uint16_t)result_8 << 8);
                            break;
                        case 0b101: // L
                            cpu->HL = (cpu->HL & 0xFF00) | result_8;
                            break;
                        case 0b110: // [HL]
                            cycles_taken +=4;
                            write_memory_8(bus, cpu->HL, result_8);
                            break;
                        case 0b111: // A
                            cpu->AF = (cpu->AF & 0x00FF) | ((uint16_t)result_8 << 8);
                            break;
                    }
            }
            break;
        case 0b10: // BLOCK 2 instructions
            uint8_t sub_block = (opcode >> 3)&0b111;
            // DEBUG_PRINTF("Sub-block: 0x%02X, Register: 0x%02X\n", sub_block, reg);
            switch(opcode & 0b111) { // source
                case 0b000: // B
                    result_8 = (cpu->BC >> 8);
                    break;
                case 0b001: // C
                    result_8 = (cpu->BC & 0xFF);
                    break;
                case 0b010: // D
                    result_8 = (cpu->DE >> 8);
                    break;
                case 0b011: // E
                    result_8 = (cpu->DE & 0xFF);
                    break;
                case 0b100: // H
                    result_8 = (cpu->HL >> 8);
                    break;
                case 0b101: // L
                    result_8 = (cpu->HL & 0xFF);
                    break;
                case 0b110: // [HL]
                    result_8 = read_memory_8(bus, cpu->HL);
                    cycles_taken = 8;
                    break;
                case 0b111: // A
                    result_8 = (cpu->AF >> 8);
                    break;
            }
            switch(sub_block) {
                case 0b000: // add a, r8
                    result = (cpu->AF >> 8) + result_8;
                    a = cpu->AF >> 8;

                    result = (uint16_t)a + result_8;

                    cpu->AF = (cpu->AF & 0x00FF) | ((result & 0xFF) << 8);

                    if ((result & 0xFF) == 0) // Zero flag
                        set_bit(cpu->AF, 7);
                    else
                        clear_bit(cpu->AF, 7);

                    clear_bit(cpu->AF, 6); // N = 0 

                    if (((a & 0x0F) + (result_8 & 0x0F)) > 0x0F) // Half Carry flag
                        set_bit(cpu->AF, 5);
                    else
                        clear_bit(cpu->AF, 5);

                    if (result > 0xFF) // Carry flag
                        set_bit(cpu->AF, 4);
                    else
                        clear_bit(cpu->AF, 4);
                    break;
                case 0b001: // adc a, r8
                    a = cpu->AF >> 8;
                    taken = check_bit(cpu->AF, 4); // old carry flag

                    result = (uint16_t)a + result_8 + taken;

                    cpu->AF = (cpu->AF & 0x00FF) | ((result & 0xFF) << 8);

                    if ((result & 0xFF) == 0) // Zero flag
                        set_bit(cpu->AF, 7);
                    else
                        clear_bit(cpu->AF, 7);

                    clear_bit(cpu->AF, 6); // N = 0

                    if (((a & 0x0F) + (result_8 & 0x0F) + taken) > 0x0F) // Half Carry flag
                        set_bit(cpu->AF, 5);
                    else
                        clear_bit(cpu->AF, 5);

                    if (result > 0xFF) // Carry flag
                        set_bit(cpu->AF, 4);
                    else
                        clear_bit(cpu->AF, 4);

                    DEBUG_PRINTF("adc a, r8\n");
                    break;
                case 0b010: // sub a, r8
                    a = cpu->AF >> 8;

                    result = (uint16_t)a - result_8;

                    cpu->AF = (cpu->AF & 0x00FF) | ((result & 0xFF) << 8);

                    if ((result & 0xFF) == 0) // Zero flag
                        set_bit(cpu->AF, 7);
                    else
                        clear_bit(cpu->AF, 7);

                    set_bit(cpu->AF, 6); // N = 1

                    if ((a & 0x0F) < (result_8 & 0x0F)) // Half Carry flag
                        set_bit(cpu->AF, 5);
                    else
                        clear_bit(cpu->AF, 5);

                    if (a < result_8) // Carry flag (borrow)
                        set_bit(cpu->AF, 4);
                    else
                        clear_bit(cpu->AF, 4);

                    DEBUG_PRINTF("sub a, r8\n");
                    break;


                case 0b011: // sbc a, r8
                    a = cpu->AF >> 8;
                    taken = check_bit(cpu->AF, 4); // old carry flag

                    result = (uint16_t)a - result_8 - taken;

                    cpu->AF = (cpu->AF & 0x00FF) | ((result & 0xFF) << 8);

                    if ((result & 0xFF) == 0) // Zero flag
                        set_bit(cpu->AF, 7);
                    else
                        clear_bit(cpu->AF, 7);

                    set_bit(cpu->AF, 6); // N = 1

                    if ((a & 0x0F) < ((result_8 & 0x0F) + taken)) // Half Carry flag
                        set_bit(cpu->AF, 5);
                    else
                        clear_bit(cpu->AF, 5);

                    if ((uint16_t)a < ((uint16_t)result_8 + taken)) // Carry flag (borrow)
                        set_bit(cpu->AF, 4);
                    else
                        clear_bit(cpu->AF, 4);

                    DEBUG_PRINTF("sbc a, r8\n");
                    break;

                case 0b100: // and a, r8
                    a = cpu->AF >> 8;

                    result_8 = a & result_8;

                    cpu->AF = (cpu->AF & 0x00FF) | ((uint16_t)result_8 << 8);

                    if (result_8 == 0) // Zero flag
                        set_bit(cpu->AF, 7);
                    else
                        clear_bit(cpu->AF, 7);

                    clear_bit(cpu->AF, 6); // N = 0
                    set_bit(cpu->AF, 5);   // H = 1
                    clear_bit(cpu->AF, 4); // C = 0

                    DEBUG_PRINTF("and a, r8\n");
                    break;

                case 0b101: // xor a, r8
                    a = cpu->AF >> 8;

                    result_8 = a ^ result_8;

                    cpu->AF = (cpu->AF & 0x00FF) | ((uint16_t)result_8 << 8);

                    if (result_8 == 0) // Zero flag
                        set_bit(cpu->AF, 7);
                    else
                        clear_bit(cpu->AF, 7);

                    clear_bit(cpu->AF, 6); // N = 0
                    clear_bit(cpu->AF, 5); // H = 0
                    clear_bit(cpu->AF, 4); // C = 0

                    DEBUG_PRINTF("xor a, r8\n");
                    break;

                case 0b110: // or a, r8
                    a = cpu->AF >> 8;

                    result_8 = a | result_8;

                    cpu->AF = (cpu->AF & 0x00FF) | ((uint16_t)result_8 << 8);

                    if (result_8 == 0) // Zero flag
                        set_bit(cpu->AF, 7);
                    else
                        clear_bit(cpu->AF, 7);

                    clear_bit(cpu->AF, 6); // N = 0
                    clear_bit(cpu->AF, 5); // H = 0
                    clear_bit(cpu->AF, 4); // C = 0

                    DEBUG_PRINTF("or a, r8\n");
                    break;

                case 0b111: // cp a, r8
                    a = cpu->AF >> 8;

                    result = (uint16_t)a - result_8;

                    if ((result & 0xFF) == 0) // Zero flag
                        set_bit(cpu->AF, 7);
                    else
                        clear_bit(cpu->AF, 7);

                    set_bit(cpu->AF, 6); // N = 1

                    if ((a & 0x0F) < (result_8 & 0x0F)) // Half Carry flag
                        set_bit(cpu->AF, 5);
                    else
                        clear_bit(cpu->AF, 5);

                    if (a < result_8) // Carry flag (borrow)
                        set_bit(cpu->AF, 4);
                    else
                        clear_bit(cpu->AF, 4);

                    DEBUG_PRINTF("cp a, r8\n");
                    break;
                default:
                    DEBUG_PRINTF("Unknown sub-block in block 2\n");
            }
            break;
        case 0b11: // Block 3 instructions
            switch(opcode & 0b111111) {
                ////// first part of the block 3 instructions -> ALU
            case 0b000110: // add a, [imm8]
                result_8 = read_memory_8(bus, cpu->PC + 1);
                a = cpu->AF >> 8;
                result = (uint16_t)a + result_8;
                cpu->AF = (cpu->AF & 0x00FF) | ((result & 0xFF) << 8);
                if ((result & 0xFF) == 0)
                    set_bit(cpu->AF, 7);
                else
                    clear_bit(cpu->AF, 7);
                clear_bit(cpu->AF, 6); // N = 0

                if (((a & 0x0F) + (result_8 & 0x0F)) > 0x0F)
                    set_bit(cpu->AF, 5);
                else
                    clear_bit(cpu->AF, 5);

                if (result > 0xFF)
                    set_bit(cpu->AF, 4);
                else
                    clear_bit(cpu->AF, 4);
                cycles_taken = 8;

                newPC = cpu->PC + 2;
                DEBUG_PRINTF("add a, imm8\n");
                break;

            case 0b001110: // adc a, imm8
                result_8 = read_memory_8(bus, cpu->PC + 1);
                a = cpu->AF >> 8;
                taken = check_bit(cpu->AF, 4);
                result = (uint16_t)a + result_8 + taken;
                cpu->AF = (cpu->AF & 0x00FF) | ((result & 0xFF) << 8);
                if ((result & 0xFF) == 0)
                    set_bit(cpu->AF, 7);
                else
                    clear_bit(cpu->AF, 7);

                clear_bit(cpu->AF, 6); // N = 0

                if (((a & 0x0F) + (result_8 & 0x0F) + taken) > 0x0F)
                    set_bit(cpu->AF, 5);
                else
                    clear_bit(cpu->AF, 5);

                if (result > 0xFF)
                    set_bit(cpu->AF, 4);
                else
                    clear_bit(cpu->AF, 4);

                cycles_taken = 8;
                newPC = cpu->PC + 2;
                DEBUG_PRINTF("adc a, imm8\n");
                break;

            case 0b010110: // sub a, imm8
                result_8 = read_memory_8(bus, cpu->PC + 1);
                a = cpu->AF >> 8;
                result = (uint16_t)a - result_8;
                cpu->AF = (cpu->AF & 0x00FF) | ((result & 0xFF) << 8);
                if ((result & 0xFF) == 0)
                    set_bit(cpu->AF, 7);
                else
                    clear_bit(cpu->AF, 7);

                set_bit(cpu->AF, 6); // N = 1

                if ((a & 0x0F) < (result_8 & 0x0F))
                    set_bit(cpu->AF, 5);
                else
                    clear_bit(cpu->AF, 5);

                if (a < result_8)
                    set_bit(cpu->AF, 4);
                else
                    clear_bit(cpu->AF, 4);

                newPC = cpu->PC + 2;
                cycles_taken = 8;
                DEBUG_PRINTF("sub a, imm8\n");
                break;

            case 0b011110: // sbc a, imm8
                result_8 = read_memory_8(bus, cpu->PC + 1);
                a = cpu->AF >> 8;
                taken = check_bit(cpu->AF, 4);
                result = (uint16_t)a - result_8 - taken;
                cpu->AF = (cpu->AF & 0x00FF) | ((result & 0xFF) << 8);
                if ((result & 0xFF) == 0)
                    set_bit(cpu->AF, 7);
                else
                    clear_bit(cpu->AF, 7);
                set_bit(cpu->AF, 6); // N = 1
                if ((a & 0x0F) < ((result_8 & 0x0F) + taken))
                    set_bit(cpu->AF, 5);
                else
                    clear_bit(cpu->AF, 5);

                if ((uint16_t)a < ((uint16_t)result_8 + taken))
                    set_bit(cpu->AF, 4);
                else
                    clear_bit(cpu->AF, 4);

                newPC = cpu->PC + 2;
                cycles_taken = 8;
                DEBUG_PRINTF("sbc a, imm8\n");
                break;

            case 0b100110: // and a, imm8
                result_8 = read_memory_8(bus, cpu->PC + 1);
                a = cpu->AF >> 8;
                result_8 = a & result_8;
                cpu->AF = (cpu->AF & 0x00FF) | ((uint16_t)result_8 << 8);
                if (result_8 == 0)
                    set_bit(cpu->AF, 7);
                else
                    clear_bit(cpu->AF, 7);
                clear_bit(cpu->AF, 6); // N = 0
                set_bit(cpu->AF, 5);   // H = 1
                clear_bit(cpu->AF, 4); // C = 0

                newPC = cpu->PC + 2;
                cycles_taken = 8;
                DEBUG_PRINTF("and a, imm8\n");
                break;

            case 0b101110: // xor a, imm8
                result_8 = read_memory_8(bus, cpu->PC + 1);
                a = cpu->AF >> 8;
                result_8 = a ^ result_8;
                cpu->AF = (cpu->AF & 0x00FF) | ((uint16_t)result_8 << 8);
                if (result_8 == 0)
                    set_bit(cpu->AF, 7);
                else
                    clear_bit(cpu->AF, 7);

                clear_bit(cpu->AF, 6); // N = 0
                clear_bit(cpu->AF, 5); // H = 0
                clear_bit(cpu->AF, 4); // C = 0

                newPC = cpu->PC + 2;
                cycles_taken = 8;
                DEBUG_PRINTF("xor a, imm8\n");
                break;

            case 0b110110: // or a, imm8
                result_8 = read_memory_8(bus, cpu->PC + 1);
                a = cpu->AF >> 8;
                result_8 = a | result_8;
                cpu->AF = (cpu->AF & 0x00FF) | ((uint16_t)result_8 << 8);
                if (result_8 == 0)
                    set_bit(cpu->AF, 7);
                else
                    clear_bit(cpu->AF, 7);
                clear_bit(cpu->AF, 6); // N = 0
                clear_bit(cpu->AF, 5); // H = 0
                clear_bit(cpu->AF, 4); // C = 0

                newPC = cpu->PC + 2;
                cycles_taken = 8;
                DEBUG_PRINTF("or a, imm8\n");
                break;

            case 0b111110: // cp a, imm8
                result_8 = read_memory_8(bus, cpu->PC + 1);
                a = cpu->AF >> 8;
                result = (uint16_t)a - result_8;
                if ((result & 0xFF) == 0)
                    set_bit(cpu->AF, 7);
                else
                    clear_bit(cpu->AF, 7);
                set_bit(cpu->AF, 6); // N = 1
                if ((a & 0x0F) < (result_8 & 0x0F))
                    set_bit(cpu->AF, 5);
                else
                    clear_bit(cpu->AF, 5);

                if (a < result_8)
                    set_bit(cpu->AF, 4);
                else
                    clear_bit(cpu->AF, 4);
                newPC = cpu->PC + 2;
                cycles_taken = 8;
                DEBUG_PRINTF("cp a, imm8\n");
                break;

                ////// second part of the block 3 instructions -> control flow
                case 0b000000: // ret Nz
                    DEBUG_PRINTF("ret Nz\n");
                    taken = !check_bit(cpu->AF, 7); // Check if Z flag is not set
                    goto ret_cond;
                case 0b001000: // ret cond
                    DEBUG_PRINTF("ret Z\n");
                    taken = check_bit(cpu->AF, 7); // Check if Z flag is set
                    goto ret_cond;
                case 0b010000: // ret cond
                    DEBUG_PRINTF("ret Nc\n");
                    taken = !check_bit(cpu->AF, 4); // Check if C flag is not set
                    goto ret_cond;
                case 0b011000: // ret cond
                    DEBUG_PRINTF("ret C\n");
                    taken = check_bit(cpu->AF, 4); // Check if C flag is set
                    // goto ret_cond;
                ret_cond:
                cycles_taken = 8;
                    if (taken) {
                        cycles_taken = 20;
                        result = pop_stack(cpu, bus);
                        newPC = result;
                    }
                    break;
                case 0b001001: // ret
                    DEBUG_PRINTF("ret\n");
                    result = pop_stack(cpu, bus);
                    newPC = result;
                    cycles_taken = 16;
                    break;
                case 0b011001: // reti
                    DEBUG_PRINTF("reti\n");
                    cpu->ime = 1; // Enable interrupts
                    result = pop_stack(cpu, bus);
                    newPC = result;
                    cycles_taken = 16;
                    break;
                case 0b000010: // JP cond, imm16
                    DEBUG_PRINTF("JP not zero, imm16\n");
                    taken = !check_bit(cpu->AF, 7); // Check if Z flag is not set
                    goto JP_cond;
                case 0b001010:
                    DEBUG_PRINTF("JP zero, imm16\n");
                    taken = check_bit(cpu->AF, 7); // Check if Z flag is set
                    goto JP_cond;
                case 0b010010:
                    DEBUG_PRINTF("JP not carry, imm16\n");
                    taken = !check_bit(cpu->AF, 4); // Check if C flag is not set
                    goto JP_cond;
                    case 0b011010:
                    DEBUG_PRINTF("JP carry, imm16\n");
                    taken = check_bit(cpu->AF, 4); // Check if C flag is set
                    // goto JP_cond;
                    JP_cond:
                    cycles_taken = 12;
                    if (taken) {
                        cycles_taken = 16;
                        result = read_memory_16(bus, cpu->PC + 1);
                        newPC = result;
                    } else {
                        newPC = cpu->PC + 3;
                    }
                    break;
                case 0b000011: // JP im16
                    DEBUG_PRINTF("jp im16\n");
                    result = read_memory_16(bus, cpu->PC + 1);
                    newPC = result;
                    cycles_taken = 16;
                    break;
                case 0b101001: // Jp HL
                    DEBUG_PRINTF("jp hl\n");
                    newPC = cpu->HL;
                    break;
                case 0b000100: // Call cond, imm16
                    DEBUG_PRINTF("call not zero, imm16\n");
                    taken = !check_bit(cpu->AF, 7); // Check if Z flag is not set
                    goto call_cond;
                case 0b001100:
                    DEBUG_PRINTF("call zero, imm16\n");
                    taken = check_bit(cpu->AF, 7); // Check if Z flag is set
                    goto call_cond;
                case 0b010100:
                    DEBUG_PRINTF("call not carry, imm16\n");
                    taken = !check_bit(cpu->AF, 4); // Check if C flag is not set
                    goto call_cond;
                case 0b011100:
                    DEBUG_PRINTF("call carry, imm16\n");
                    taken = check_bit(cpu->AF, 4); // Check if C flag is set
                    // goto call_cond;
                call_cond:
                    cycles_taken = 12;
                    if (taken) {
                        cycles_taken = 24;
                        result = read_memory_16(bus, cpu->PC + 1);
                        push_stack(cpu, cpu->PC + 3, bus);
                        newPC = result;
                    } else { 
                        newPC = cpu->PC + 3;
                    }
                    break;
                case 0b001101:
                    DEBUG_PRINTF("call im16\n");
                    result = read_memory_16(bus, cpu->PC + 1);
                    push_stack(cpu, cpu->PC + 3, bus);
                    cycles_taken = 24;
                    newPC = result;
                    break;
                case 0b000111: // rst tgt3
                case 0b001111:
                case 0b010111:
                case 0b011111:
                case 0b100111:
                case 0b101111:
                case 0b110111:
                case 0b111111:
                    push_stack(cpu, cpu->PC + 1, bus);
                    newPC = opcode & 0x38;
                    cycles_taken = 16;
                    DEBUG_PRINTF("rst tgt3\n");
                    break;

                ////// third part of the block 3 instructions -> misc
                case 0b000001: // pop rp2
                case 0b010001: // pop rp2
                case 0b100001: // pop rp2
                case 0b110001: // pop rp2
                    DEBUG_PRINTF("pop rp2\n");
                    result = pop_stack(cpu, bus);
                    switch ((opcode >> 4) & 0b11) {
                        case 0b00:
                            cpu->BC = result;
                            break;

                        case 0b01:
                            cpu->DE = result;
                            break;

                        case 0b10:
                            cpu->HL = result;
                            break;

                        case 0b11:
                            cpu->AF = result & 0xFFF0;
                            break;
                    }
                    cycles_taken = 12;
                    break;
                case 0b000101: // push rp2
                case 0b010101: // push rp2
                case 0b100101: // push rp2
                case 0b110101: // push rp2
                    DEBUG_PRINTF("push rp2\n");
                    switch ((opcode >> 4) & 0b11) {
                        case 0b00:
                            push_stack(cpu, cpu->BC, bus);
                            break;

                        case 0b01:
                            push_stack(cpu, cpu->DE, bus);
                            break;

                        case 0b10:
                            push_stack(cpu, cpu->HL, bus);
                            break;

                        case 0b11:
                            push_stack(cpu, cpu->AF & 0xFFF0, bus);
                            break;
                    }
                    cycles_taken = 16;
                    break;
                /// fourth part of the block 3 instructions -> loads
                case 0b100010: // ldh [c], a
                    DEBUG_PRINTF("LD [c], a\n");
                    write_memory_8(bus, 0xFF00 + (cpu->BC & 0xFF), cpu->AF >> 8);
                    cycles_taken = 8;
                    break;
                case 0b100000: // ldh [imm8], a
                    DEBUG_PRINTF("LD [imm8], a\n");
                    write_memory_8(bus, 0xFF00 + read_memory_8(bus, cpu->PC + 1), cpu->AF >> 8);
                    newPC = cpu->PC + 2;
                    cycles_taken = 12;
                    break;
                case 0b101010: // LD [imm16], a
                    DEBUG_PRINTF("LD [imm16], a\n");
                    result = read_memory_16(bus, cpu->PC + 1);
                    write_memory_8(bus, result, cpu->AF >> 8);
                    newPC = cpu->PC + 3;
                    cycles_taken = 16;
                    break;
                case 0b110010: // ldh a, [c]
                    DEBUG_PRINTF("LD a, [c]\n");
                    cpu->AF = (cpu->AF & 0x00FF) | ((uint16_t)read_memory_8(bus, 0xFF00 + (cpu->BC & 0xFF)) << 8);
                    cycles_taken = 8;
                    break;
                case 0b110000: // ldh a, [imm8]
                    result_8 = read_memory_8(bus, cpu->PC + 1);
                    cpu->AF = (cpu->AF & 0x00FF) | ((uint16_t)read_memory_8(bus, 0xFF00 + result_8) << 8);
                    DEBUG_PRINTF("ldh a, [imm8]\n");
                    newPC = cpu->PC + 2;
                    cycles_taken = 12;
                    break;
                case 0b111010: // LD a, [imm16]
                    DEBUG_PRINTF("LD a, [imm16]\n");
                    result = read_memory_16(bus, cpu->PC + 1);
                    cpu->AF = (cpu->AF & 0x00FF) | ((uint16_t)read_memory_8(bus, result) << 8);
                    newPC = cpu->PC + 3;
                    cycles_taken = 16;
                    break;

                // fifth part of the block 3 instructions -> misc
                case 0b101000: // add sp, imm8
                    result_8 = read_memory_8(bus, cpu->PC + 1);
                    offset = (int8_t)result_8;

                    result = cpu->SP + offset;

                    clear_bit(cpu->AF, 7); // Z = 0
                    clear_bit(cpu->AF, 6); // N = 0

                    if (((cpu->SP & 0x0F) + (result_8 & 0x0F)) > 0x0F)
                        set_bit(cpu->AF, 5);
                    else
                        clear_bit(cpu->AF, 5);

                    if (((cpu->SP & 0xFF) + result_8) > 0xFF)
                        set_bit(cpu->AF, 4);
                    else
                        clear_bit(cpu->AF, 4);

                    cpu->SP = result;

                    newPC = cpu->PC + 2;
                    cycles_taken = 16;
                    DEBUG_PRINTF("add sp, imm8\n");
                    break;
                case 0b111000: // LD HL, SP+imm8
                    result_8 = read_memory_8(bus, cpu->PC + 1);
                    offset = (int8_t)result_8;

                    result = cpu->SP + offset;

                    clear_bit(cpu->AF, 7); // Z = 0
                    clear_bit(cpu->AF, 6); // N = 0

                    if (((cpu->SP & 0x0F) + (result_8 & 0x0F)) > 0x0F)
                        set_bit(cpu->AF, 5);
                    else
                        clear_bit(cpu->AF, 5);

                    if (((cpu->SP & 0xFF) + result_8) > 0xFF)
                        set_bit(cpu->AF, 4);
                    else
                        clear_bit(cpu->AF, 4);

                    cpu->HL = result;

                    newPC = cpu->PC + 2;
                    cycles_taken = 12;
                    DEBUG_PRINTF("LD HL, SP+imm8\n");
                    break;
                case 0b111001: // LD SP, HL
                    cpu->SP = cpu->HL;
                    cycles_taken = 8;
                    DEBUG_PRINTF("LD SP, HL\n");
                    break;
                // sixth part of the block 3 instructions -> ei / di
                case 0b110011: // DI
                    cpu->ime = 0;
                    cpu->ime_enable_delay = 0;
                    break;
                case 0b111011: // ei
                    DEBUG_PRINTF("ei\n");
                    cpu->ime_enable_delay = 2;
                    break;
                case (0xCB & 0b111111):
                    cycles_taken = decode_cb(cpu, read_memory_8(bus, cpu->PC + 1), bus);
                    newPC = cpu->PC + 2;
                    break;
                default:
                    DEBUG_PRINTF("Unknown instruction in block 3\n");
                    not_implemented("Unknown instruction in block 3");
                    break;
            }
            break;
    }
    cpu->PC = newPC;
    return cycles_taken;
}

uint8_t decode_cb(CPU *cpu, uint8_t opcode, Bus *bus) {
    uint8_t block = opcode >> 6;
    uint8_t operation = (opcode >> 3) & 0b111;
    uint8_t reg = opcode & 0b111;

    uint8_t value;
    uint8_t result;
    uint8_t carry;
    uint8_t cycles_taken;
    switch (reg) {
        case 0b000: // B
            value = cpu->BC >> 8;
            break;

        case 0b001: // C
            value = cpu->BC & 0xFF;
            break;

        case 0b010: // D
            value = cpu->DE >> 8;
            break;

        case 0b011: // E
            value = cpu->DE & 0xFF;
            break;

        case 0b100: // H
            value = cpu->HL >> 8;
            break;

        case 0b101: // L
            value = cpu->HL & 0xFF;
            break;

        case 0b110: // [HL]
            value = read_memory_8(bus, cpu->HL);
            break;

        case 0b111: // A
            value = cpu->AF >> 8;
            break;
    }
    if (reg == 6) {
        cycles_taken = (block == 1) ? 12 : 16;
    } else {
        cycles_taken = 8;
    }
    switch (block) {
        case 0b00:
            switch (operation) {
                case 0b000: // RLC
                    carry = value >> 7;
                    result = (value << 1) | carry;
                    break;

                case 0b001: // RRC
                    carry = value & 1;
                    result = (value >> 1) | (carry << 7);
                    break;

                case 0b010: // RL
                    carry = value >> 7;
                    result = (value << 1) | check_bit(cpu->AF, 4);
                    break;

                case 0b011: // RR
                    carry = value & 1;
                    result = (value >> 1) | (check_bit(cpu->AF, 4) << 7);
                    break;

                case 0b100: // SLA
                    carry = value >> 7;
                    result = value << 1;
                    break;

                case 0b101: // SRA
                    carry = value & 1;
                    result = (value >> 1) | (value & 0x80);
                    break;

                case 0b110: // SWAP
                    carry = 0;
                    result = (value << 4) | (value >> 4);
                    break;

                case 0b111: // SRL
                    carry = value & 1;
                    result = value >> 1;
                    break;
            }

            if (result == 0)
                set_bit(cpu->AF, 7);
            else
                clear_bit(cpu->AF, 7);

            clear_bit(cpu->AF, 6);
            clear_bit(cpu->AF, 5);

            if (carry)
                set_bit(cpu->AF, 4);
            else
                clear_bit(cpu->AF, 4);

            break;

        case 0b01: // BIT b,r8
            if (check_bit(value, operation))
                clear_bit(cpu->AF, 7);
            else
                set_bit(cpu->AF, 7);

            clear_bit(cpu->AF, 6);
            set_bit(cpu->AF, 5);

            return cycles_taken;;

        case 0b10: // RES b,r8
            result = value & ~(1 << operation);
            break;

        case 0b11: // SET b,r8
            result = value | (1 << operation);
            break;
    }

    switch (reg) {
        case 0b000: // B
            cpu->BC = (cpu->BC & 0x00FF) | ((uint16_t)result << 8);
            break;

        case 0b001: // C
            cpu->BC = (cpu->BC & 0xFF00) | result;
            break;

        case 0b010: // D
            cpu->DE = (cpu->DE & 0x00FF) | ((uint16_t)result << 8);
            break;

        case 0b011: // E
            cpu->DE = (cpu->DE & 0xFF00) | result;
            break;

        case 0b100: // H
            cpu->HL = (cpu->HL & 0x00FF) | ((uint16_t)result << 8);
            break;

        case 0b101: // L
            cpu->HL = (cpu->HL & 0xFF00) | result;
            break;

        case 0b110: // [HL]
            write_memory_8(bus, cpu->HL, result);
            cycles_taken = 16; // Memory access takes longer
            break;

        case 0b111: // A
            cpu->AF = (cpu->AF & 0x00FF) | ((uint16_t)result << 8);
            break;
    }
    return cycles_taken;
}

uint8_t fetch(CPU *cpu, uint8_t *memory) {
    // Fetch the next instruction from memory at the program counter (PC)
    // For now, we will just INC  rement the PC to simulate fetching an instruction
    uint8_t instruction = memory[cpu->PC];
    return instruction;
}
void push_stack(CPU *cpu, uint16_t value, Bus *bus) {
    // Push a 16-bit value onto the stack
    cpu->SP -= 2; // Decrement stack pointer by 2 (stack grows downwards)
    write_memory_16(bus, cpu->SP, value); // Write the 16-bit value to memory at the stack pointer
}
uint16_t pop_stack(CPU *cpu, Bus *bus) {
    // Pop a 16-bit value from the stack
    uint16_t value = read_memory_16(bus, cpu->SP); // Read the 16-bit value from memory at the stack pointer
    cpu->SP += 2; // Increment stack pointer by 2 (stack grows downwards)
    return value;
}

uint8_t get_pending_interrupts(Bus *bus)
{
    uint8_t ie = read_memory_8(bus, 0xFFFF);
    uint8_t interrupt_flags = read_memory_8(bus, 0xFF0F);

    return ie & interrupt_flags & 0x1F;
}

uint8_t service_interrupt(CPU *cpu, Bus *bus, uint8_t pending)
{
    if (!cpu->ime || pending == 0)
        return 0;

    cpu->ime = 0;
    cpu->halted = 0;

    uint8_t bit;
    uint16_t vector;

    if (pending & (1 << 0)) {
        bit = 0;
        vector = 0x0040;
    }
    else if (pending & (1 << 1)) {
        bit = 1;
        vector = 0x0048;
    }
    else if (pending & (1 << 2)) {
        bit = 2;
        vector = 0x0050;
    }
    else if (pending & (1 << 3)) {
        bit = 3;
        vector = 0x0058;
    }
    else {
        bit = 4;
        vector = 0x0060;
    }

    // Clear this interrupt request from IF
    uint8_t interrupt_flags = read_memory_8(bus, 0xFF0F);
    interrupt_flags &= ~(1 << bit);
    write_memory_8(bus, 0xFF0F, interrupt_flags);

    // Save where we were
    push_stack(cpu, cpu->PC, bus);

    // Jump to ISR
    cpu->PC = vector;

    return 1;
}

void reset(CPU *cpu){
    cpu->AF =0x00;
    cpu->BC =0x00;
    cpu->DE =0x00;
    cpu->HL =0x00;
    cpu->SP =0x00;
    cpu->PC = 0x100; // Start execution at address 0x100
    cpu->SP = 0xFFFE; // Initialize stack pointer
    cpu->cycles = 0; // Reset cycle count
    cpu->ime = 0; // Disable interrupts
    cpu->halted = 0; // Not halted
    cpu->ime_enable_delay = 0; // No delay for enabling interrupts
}

void print_registers(CPU *cpu) {
    DEBUG_PRINTF("Cycles: %u AF: 0x%04X, BC: 0x%04X, DE: 0x%04X, HL: 0x%04X, SP: 0x%04X, PC: 0x%04X\n",
           cpu->cycles, cpu->AF, cpu->BC, cpu->DE, cpu->HL, cpu->SP, cpu->PC);
}


void not_implemented(const char *instruction) {
    #ifdef DEBUG
    printf("\033[0;31m");
    printf("Instruction %s not implemented yet.\n", instruction);
    printf("\033[0m");
    exit(1);
    #endif
}
