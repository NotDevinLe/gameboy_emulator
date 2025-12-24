#include "bus.h"
#include "cart.h"
#include <cstdint>

static uint8_t wram[0x2000]; // C000-DFFF (8KB)
static uint8_t hram[0x7F];   // FF80-FFFE (127B)
static uint8_t ie = 0;       // FFFF

uint8_t bus_read(uint16_t addr) {
    // ROM / cartridge
    if (addr < 0x8000) {
        return cart_read(addr);
    }

    // WRAM
    if (addr >= 0xC000 && addr <= 0xDFFF) {
        return wram[addr - 0xC000];
    }

    // HRAM
    if (addr >= 0xFF80 && addr <= 0xFFFE) {
        return hram[addr - 0xFF80];
    }

    // IE
    if (addr == 0xFFFF) {
        return ie;
    }

    return 0xFF;
}

void bus_write(uint16_t addr, uint8_t val) {
    // Writes to ROM range are used for MBC control later.
    if (addr < 0x8000) {
        cart_write(addr, val);
        return;
    }

    // WRAM
    if (addr >= 0xC000 && addr <= 0xDFFF) {
        wram[addr - 0xC000] = val;
        return;
    }

    // HRAM
    if (addr >= 0xFF80 && addr <= 0xFFFE) {
        hram[addr - 0xFF80] = val;
        return;
    }

    // IE
    if (addr == 0xFFFF) {
        ie = val;
        return;
    }
}