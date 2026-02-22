#include "bus.h"
#include "cart.h"
#include "ram.h"
#include "io.h"
#include "cpu.h"
#include "emu.h"
#include <cstdint>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>

extern Ram ram;

uint8_t bus_read(uint16_t addr) {
    // ROM / cartridge
    if (addr < 0x8000) {
        return cart_read(addr);
    }
    else if (addr < 0xA000) { // vram
        return vram_read(addr - 0x8000);
    }
    else if (addr < 0xC000) { // external ram (cartridge RAM)
        return cart_read(addr);
    }
    else if (addr < 0xE000) { // work ram
        return wram_read(addr - 0xC000);
    }
    else if (addr < 0xFE00) { // echo ram
        return wram_read(addr - 0xE000);
    }
    else if (addr < 0xFEA0) { // OAM (Object Attribute Memory)
        return ram.oam[addr - 0xFE00];
    }
    else if (addr < 0xFF00) { // unusable memory area
        return 0xFF;
    }
    else if (addr < 0xFF80) { // io registers
        return io_read(addr);
    }
    else if (addr < 0xFFFF) { // high ram
        return hram_read(addr - 0xFF80);
    }
    else if (addr == 0xFFFF) { // IE
        return ram.ie;
    }

    return 0xFF;
}

void bus_write(uint16_t addr, uint8_t val) {
    // ROM / cartridge
    if (addr < 0x8000) {
        cart_write(addr, val);
    }
    else if (addr < 0xA000) { // vram
        vram_write(addr - 0x8000, val);
    }
    else if (addr < 0xC000) { // external ram (cartridge RAM)
        cart_write(addr, val);
    }
    else if (addr < 0xE000) { // work ram
        wram_write(addr - 0xC000, val);
    }
    else if (addr < 0xFE00) { // echo ram - mirrors WRAM (0xC000-0xDDFF)
        wram_write(addr - 0xE000, val);
    }
    else if (addr < 0xFEA0) { // OAM (Object Attribute Memory)
        ram.oam[addr - 0xFE00] = val;
    }
    else if (addr < 0xFF00) { // unusable memory area

    }
    else if (addr < 0xFF80) { // io registers
        io_write(addr, val);
    }
    else if (addr < 0xFFFF) { // high ram
        hram_write(addr - 0xFF80, val);
    }
    else if (addr == 0xFFFF) { // IE
        ram.ie = val;
    }
}

uint16_t bus_read16(uint16_t addr) {
    uint16_t lo = bus_read(addr);
    uint16_t hi = bus_read(addr + 1);
    return (hi << 8) | lo;
}

void bus_write16(uint16_t addr, uint16_t val) {
    // Game Boy is little-endian: low byte at addr, high byte at addr+1.
    bus_write(addr + 1, (val >> 8) & 0xFF);
    bus_write(addr,     val & 0xFF);
}