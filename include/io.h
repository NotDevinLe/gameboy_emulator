#pragma once

#include <cstdint>

// Read/write Game Boy I/O registers (0xFF00–0xFF7F).
uint8_t io_read(uint16_t addr);
void    io_write(uint16_t addr, uint8_t value);

