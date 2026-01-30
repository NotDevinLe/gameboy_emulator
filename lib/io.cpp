#include "io.h"
#include "interrupt.h"
#include "cpu.h"
#include "timer.h"
#include "emu.h"
#include <cstdio>
#include <cstdint>

io_context io;

void io_init() {
    io.joypad = 0xCF;
    io.serial_data[0] = 0x00; 
    io.serial_data[1] = 0x7E;
    io.if_reg = 0xE1; 
    io.lcdc = 0x91; 
    io.ly = 0x00; 
}

uint8_t io_read(uint16_t addr) {
    if (addr == 0xFF00) {
        return io.joypad;
    }
    else if (addr == 0xFF01) {
        return io.serial_data[0];
    }
    else if (addr == 0xFF02) {
        return io.serial_data[1];
    }
    else if (addr >= 0xFF04 && addr <= 0xFF07) {
        return timer_read(addr);
    }
    else if (addr == 0xFF0F) {
        return io.if_reg;
    }
    else if (addr == 0xFF40) {
        return io.lcdc;
    }
    else if (addr == 0xFF44) {
        return io.ly;
    }
    return 0xFF;
}

void io_write(uint16_t addr, uint8_t val) {
    if (addr == 0xFF00) {
        io.joypad = val;
    }
    else if (addr == 0xFF01) {
        io.serial_data[0] = val;
    }
    else if (addr == 0xFF02) {
        io.serial_data[1] = val;
        if (val == 0x81) {
            std::printf("%c", io.serial_data[0]);
            fflush(stdout);
            io.serial_data[1] = 0;
        }
    }
    else if (addr >= 0xFF04 && addr <= 0xFF07) {
        timer_write(addr, val);
    }
    else if (addr == 0xFF0F) {
        io.if_reg = val;
    }
    else if (addr == 0xFF40) {
        io.lcdc = val;
    }
    else if (addr == 0xFF44) {
        return;
    }
}