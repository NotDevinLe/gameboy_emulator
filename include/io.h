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
    
    // Sound registers (APU)
    uint8_t nr10, nr11, nr12, nr14;  // Channel 1
    uint8_t nr21, nr22, nr24;        // Channel 2
    uint8_t nr30, nr31, nr32, nr34;  // Channel 3
    uint8_t nr41, nr42, nr43, nr44;  // Channel 4
    uint8_t nr50, nr51, nr52;        // Sound control
    
    // PPU registers
    uint8_t stat;       // LCD Status (0xFF41)
    uint8_t scy, scx;   // Scroll Y, Scroll X
    uint8_t lyc;        // LY Compare
    uint8_t bgp;        // BG Palette
    uint8_t dma;        // DMA transfer register (0xFF46)
    uint8_t obp0, obp1; // Object Palette 0, 1
    uint8_t wy, wx;     // Window Y, Window X

} io_context;

extern io_context io;

// Read/write Game Boy I/O registers (0xFF00–0xFF7F).
uint8_t io_read(uint16_t addr);
void    io_write(uint16_t addr, uint8_t value);
void io_init();

// Joypad button indices
enum joypad_btn {
    BTN_RIGHT  = 0,
    BTN_LEFT   = 1,
    BTN_UP     = 2,
    BTN_DOWN   = 3,
    BTN_A      = 4,
    BTN_B      = 5,
    BTN_SELECT = 6,
    BTN_START  = 7,
};

void joypad_press(joypad_btn btn);
void joypad_release(joypad_btn btn);
