#pragma once

#include <cstdint>
#include <stdint.h>

// io.h
typedef struct {
    uint8_t joypad; 
    uint8_t serial_data[2]; 
    uint8_t if_reg;
    uint8_t lcdc;
    uint8_t ly;

} io_context;

extern io_context io;

// Read/write Game Boy I/O registers (0xFF00–0xFF7F).
uint8_t io_read(uint16_t addr);
void    io_write(uint16_t addr, uint8_t value);
void io_init();

