#pragma once
#include <cstdint>
#include <stdint.h>

struct Sprite {
    int x;
    int y;
    uint8_t tile_id;
    uint8_t attribute_flags;
};

// Initializes the PPU
// initial mode is 2 (OAM Scan)
extern uint32_t screen[160 * 144];

void ppu_init();
void ppu_step(uint8_t cycles);
void ppu_oam_write(uint16_t address, uint8_t value);