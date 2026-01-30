#pragma once
#include <cstdint>
#include <stdint.h>

void ppu_init();
void ppu_step(uint8_t cycles);