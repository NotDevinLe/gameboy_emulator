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

uint8_t cpu_handle_interrupts(cpu_state *ctx);
void request_interrupt(uint8_t type);