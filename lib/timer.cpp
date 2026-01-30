#include "timer.h"
#include "interrupt.h"

timer_ctx timer;

void timer_init() {
    timer.counter = 0;
    timer.tima = 0;
    timer.tma = 0;
    timer.tac = 0;
    timer.prev_and_result = false;
    timer.interrupt_pending = false;
}

void timer_tick() {
    // each tick is called every 4 machine cycles
    timer.counter+= 4;

    bool current_signal = (timer.tac & 0x04) != 0;

    if (current_signal) {
        current_signal = (timer.counter & get_system_bit_mask(timer.tac)) != 0;
    }

    if (timer.prev_and_result && !current_signal) {
        tima_increment();
    }

    timer.prev_and_result = current_signal;
}

uint8_t timer_read(uint16_t address) {
    switch (address) {
        case 0xFF04:
            return get_div();
        case 0xFF05:
            return timer.tima;
        case 0xFF06:
            return timer.tma;
        case 0xFF07:
            return timer.tac | 0xF8;
    }
    return 0xFF;
}

void timer_write(uint16_t address, uint8_t value) {
    switch (address) {
        case 0xFF04:
            timer_write_div();
            break;
        case 0xFF05:
            timer.tima = value;
            break;
        case 0xFF06:
            timer.tma = value;
            break;
        case 0xFF07:
            timer_write_tac(value);
            break;
    }
}

uint16_t get_system_bit_mask(uint8_t tac) {
    // Extract the bottom 2 bits (Clock Select)
    switch (tac & 0x03) {
        case 0x00: return 0x0200; // Bit 9 (4096 Hz)
        case 0x01: return 0x0008; // Bit 3 (262144 Hz)
        case 0x02: return 0x0020; // Bit 5 (65536 Hz)
        case 0x03: return 0x0080; // Bit 7 (16384 Hz)
        default:   return 0x0200; // Should never reach here
    }
}

void tima_increment() {
    timer.tima++;
    if (timer.tima == 0) {
        timer.tima = timer.tma;
        timer.interrupt_pending = true;
    }
}

void timer_write_tac(uint8_t value) {
    bool old_signal = timer.prev_and_result;
    
    timer.tac = value;

    bool enable = (timer.tac & 0x04) != 0;
    bool bit_state = (timer.counter & get_system_bit_mask(timer.tac)) != 0;
    bool current_signal = enable && bit_state;
    
    timer.prev_and_result = current_signal;

    if (old_signal && !current_signal) {
        tima_increment();
    }
}

void timer_write_div() {
    bool old_signal = timer.prev_and_result;
    timer.counter = 0;
    timer.prev_and_result = false;

    if (old_signal) {
        tima_increment();
    }
}

uint8_t get_div() {
    return (timer.counter >> 8) & 0xFF;
}