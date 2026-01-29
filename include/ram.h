#pragma once
#include <cstdint>

class Ram {
    public:
        uint8_t vram[2][0x2000];
        uint8_t wram[2][0x1000];
        uint8_t oam[0xA0];
        uint8_t hram[0x7F];
        uint8_t ie;
};

// Global RAM instance, defined in ram.cpp
extern Ram ram;

// Initialize the global RAM instance to zeroed memory
void ram_init();

uint8_t vram_read(uint16_t index);
void vram_write(uint16_t index, uint8_t val);

uint8_t wram_read(uint16_t index);
void wram_write(uint16_t index, uint8_t val);

uint8_t hram_read(uint16_t index);
void hram_write(uint16_t index, uint8_t val);
