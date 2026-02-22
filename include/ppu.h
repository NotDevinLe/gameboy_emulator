#pragma once
#include <cstdint>
#include <stdint.h>

void ppu_init();
void ppu_step(uint8_t cycles);
void ppu_oam_write(uint16_t address, uint8_t value);