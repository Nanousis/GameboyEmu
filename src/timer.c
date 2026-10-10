#include "timer.h"
#include "bus.h"

uint16_t timer_period(uint8_t tac)
{
    switch (tac & 0x03) {
        case 0:
            return 1024;

        case 1:
            return 16;

        case 2:
            return 64;

        case 3:
            return 256;
    }

    return 1024;
}
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

uint8_t timer_input(const Timer *timer)
{
    if (!(timer->TAC & 0x04))
        return 0;

    unsigned bit;

    switch (timer->TAC & 0x03) {
        case 0: bit = 9; break;
        case 1: bit = 3; break;
        case 2: bit = 5; break;
        case 3: bit = 7; break;
    }

    return (timer->div_counter >> bit) & 1;
}
void timer_tick_one(Bus *bus)
{
    Timer *timer = &bus->timer;

    uint8_t old_signal = timer_input(timer);

    timer->div_counter++;

    uint8_t new_signal = timer_input(timer);

    if (old_signal && !new_signal) {
        if (timer->TIMA == 0xFF) {
            timer->TIMA = timer->TMA;

            // Request timer interrupt
            uint8_t flags = read_memory_8(bus, 0xFF0F);
            write_memory_8(bus, 0xFF0F, flags | (1 << 2));
        }
        else {
            timer->TIMA++;
        }
    }
}
void timer_tick(Bus *bus, uint8_t cycles)
{
    for (uint8_t i = 0; i < cycles; i++) {
        timer_tick_one(bus);
    }
}