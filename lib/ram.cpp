#include "ram.h"
#include <cstring>

Ram ram;

void ram_init() {
    memset(ram.vram, 0, sizeof(ram.vram));
    memset(ram.wram, 0, sizeof(ram.wram));
    memset(ram.oam, 0, sizeof(ram.oam));
    memset(ram.hram, 0, sizeof(ram.hram));
    
    // IE register (0xFFFF) - post-boot state is 0x00
    ram.ie = 0x00;
}

uint8_t vram_read(uint16_t index) {
    return ram.vram[0][index];
}

void vram_write(uint16_t index, uint8_t val) {
    ram.vram[0][index] = val;
}

uint8_t wram_read(uint16_t index) {
    // Work RAM is 8KB, arranged as two 4KB banks.
    // The bus passes an index in the range [0, 0x1FFF], where:
    //   0x0000-0x0FFF -> bank 0
    //   0x1000-0x1FFF -> bank 1 (switchable on real hardware; we always use bank 1 here)
    uint8_t bank = (index >= 0x1000) ? 1 : 0;
    uint16_t offset = index & 0x0FFF;
    return ram.wram[bank][offset];
}

void wram_write(uint16_t index, uint8_t val) {
    uint8_t bank = (index >= 0x1000) ? 1 : 0;
    uint16_t offset = index & 0x0FFF;
    ram.wram[bank][offset] = val;
}

uint8_t hram_read(uint16_t index) {
    return ram.hram[index];
}

void hram_write(uint16_t index, uint8_t val) {
    ram.hram[index] = val;
}