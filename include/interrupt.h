#pragma once

#include <cpu.h>
#include <cstdint>

typedef enum {
    IT_VBLANK = 1,
    IT_LCD_STAT = 2,
    IT_TIMER = 4,
    IT_SERIAL = 8,
    IT_JOYPAD = 16
} interrupt_type;

void cpu_request_interrupt(interrupt_type t);
void interrupt_request(interrupt_type t);  // Alias for cpu_request_interrupt
uint8_t interrupt_get_if();
void interrupt_set_if(uint8_t value);

void cpu_handle_interrupts(cpu_state *ctx);