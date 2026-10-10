#pragma once

#include <stdint.h>

typedef struct Bus Bus;


typedef struct Timer{
    uint16_t div_counter;

    uint8_t TIMA;
    uint8_t TMA;
    uint8_t TAC;

    uint16_t tima_counter;
} Timer;

uint16_t timer_period(uint8_t tac);
// simple implementation
// void timer_tick(Bus *bus, uint8_t cycles)
// {
//     Timer *timer = &bus->timer;

//     timer->div_counter += cycles;

//     if (!(timer->TAC & 0x04))
//         return;

//     timer->tima_counter += cycles;

//     uint16_t period = timer_period(timer->TAC);

//     while (timer->tima_counter >= period) {
//         timer->tima_counter -= period;

//         if (timer->TIMA == 0xFF) {
//             timer->TIMA = timer->TMA;
//             // Request timer interrupt
//             bus->IF |= 0x04;
//         }
//         else {
//             timer->TIMA++;
//         }
//     }
// }

uint8_t timer_input(const Timer *timer);
void timer_tick_one(Bus *bus);
void timer_tick(Bus *bus, uint8_t cycles);