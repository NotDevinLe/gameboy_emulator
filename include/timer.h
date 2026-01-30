#pragma once
#include <cstdint>

typedef struct {
    uint16_t counter;
    uint8_t tima, tma, tac;
    bool prev_and_result, interrupt_pending;
} timer_ctx;

void timer_init();
void timer_tick();
uint8_t timer_read(uint16_t address);
void timer_write(uint16_t address, uint8_t value);
uint8_t get_div();
void timer_write_div();
void timer_write_tac(uint8_t value);
void tima_increment();
uint16_t get_system_bit_mask(uint8_t tac);