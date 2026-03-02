#include "io.h"
#include "interrupt.h"
#include "cpu.h"
#include "timer.h"
#include "emu.h"
#include "dma.h"
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>

io_context io;

// Joypad button state: 8 bits, one per button (1 = pressed, 0 = released)
// Bits 0-3: Right, Left, Up, Down  (d-pad)
// Bits 4-7: A, B, Select, Start    (action)
static uint8_t joypad_state = 0x00;  // all released

void joypad_press(joypad_btn btn) {
    joypad_state |= (1 << btn);
    // Request joypad interrupt (IF bit 4)
    uint8_t if_reg = io.if_reg;
    io.if_reg = if_reg | 0x10;
}

void joypad_release(joypad_btn btn) {
    joypad_state &= ~(1 << btn);
}

void io_init() {
    // Joypad
    io.joypad = 0xCF;
    
    // Serial
    io.serial_data[0] = 0x00; 
    io.serial_data[1] = 0x7E;
    
    // Interrupts
    io.if_reg = 0xE1; 
    
    // PPU
    io.lcdc = 0x91; 
    io.stat = 0x85;  // Mode 1 (VBlank) + LYC=LY flag set initially
    io.ly = 0x00; 
    io.scy = 0x00;
    io.scx = 0x00;
    io.lyc = 0x00;
    io.bgp = 0xFC;
    io.dma = 0xFF;
    io.obp0 = 0xFF;
    io.obp1 = 0xFF;
    io.wy = 0x00;
    io.wx = 0x00;
    
    // Sound (APU) - Channel 1
    io.nr10 = 0x80;
    io.nr11 = 0xBF;
    io.nr12 = 0xF3;
    io.nr14 = 0xBF;
    
    // Sound - Channel 2
    io.nr21 = 0x3F;
    io.nr22 = 0x00;
    io.nr24 = 0xBF;
    
    // Sound - Channel 3
    io.nr30 = 0x7F;
    io.nr31 = 0xFF;
    io.nr32 = 0x9F;
    io.nr34 = 0xBF;
    
    // Sound - Channel 4
    io.nr41 = 0xFF;
    io.nr42 = 0x00;
    io.nr43 = 0x00;
    io.nr44 = 0xBF;
    
    // Sound - Control
    io.nr50 = 0x77;
    io.nr51 = 0xF3;
    io.nr52 = 0xF1;  // DMG: bit 7 set means sound enabled
}

uint8_t io_read(uint16_t addr) {
    if (addr == 0xFF00) {
        // Joypad register (P1/JOYP):
        //   Bits 7-6: unused, always 1
        //   Bit 5: 0 = select action buttons (Start/Select/B/A)
        //   Bit 4: 0 = select d-pad (Down/Up/Left/Right)
        //   Bits 3-0: button state (0 = pressed, 1 = not pressed)
        uint8_t result = 0xCF; // all unselected, no buttons
        if (!(io.joypad & 0x10)) {
            // D-pad selected: bits 0-3 of joypad_state = Right,Left,Up,Down
            result = 0xC0 | (io.joypad & 0x30) | (~joypad_state & 0x0F);
        }
        if (!(io.joypad & 0x20)) {
            // Action buttons selected: bits 4-7 of joypad_state = A,B,Select,Start
            result = 0xC0 | (io.joypad & 0x30) | (~(joypad_state >> 4) & 0x0F);
        }
        return result;
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
    else if (addr == 0xFF41) {
        return io.stat;
    }
    else if (addr == 0xFF44) {
        return io.ly;
    }
    else if (addr == 0xFF46) {
        return io.dma;
    }
    // Sound registers
    else if (addr == 0xFF10) return io.nr10;
    else if (addr == 0xFF11) return io.nr11;
    else if (addr == 0xFF12) return io.nr12;
    else if (addr == 0xFF14) return io.nr14;
    else if (addr == 0xFF16) return io.nr21;
    else if (addr == 0xFF17) return io.nr22;
    else if (addr == 0xFF19) return io.nr24;
    else if (addr == 0xFF1A) return io.nr30;
    else if (addr == 0xFF1B) return io.nr31;
    else if (addr == 0xFF1C) return io.nr32;
    else if (addr == 0xFF1E) return io.nr34;
    else if (addr == 0xFF20) return io.nr41;
    else if (addr == 0xFF21) return io.nr42;
    else if (addr == 0xFF22) return io.nr43;
    else if (addr == 0xFF23) return io.nr44;
    else if (addr == 0xFF24) return io.nr50;
    else if (addr == 0xFF25) return io.nr51;
    else if (addr == 0xFF26) return io.nr52;
    // PPU registers
    else if (addr == 0xFF42) return io.scy;
    else if (addr == 0xFF43) return io.scx;
    else if (addr == 0xFF45) return io.lyc;
    else if (addr == 0xFF47) return io.bgp;
    else if (addr == 0xFF48) return io.obp0;
    else if (addr == 0xFF49) return io.obp1;
    else if (addr == 0xFF4A) return io.wy;
    else if (addr == 0xFF4B) return io.wx;
    return 0xFF;
}

void io_write(uint16_t addr, uint8_t val) {
    if (addr == 0xFF02 && val == 0x81) {
        char c = io.serial_data[0];          // FF01
        putchar(c);
        fflush(stdout);
        io.serial_data[1] = 0x00;            // transfer complete
        return;
    }
    if (addr == 0xFF00) {
        // Only bits 4-5 are writable (button group selection)
        io.joypad = (io.joypad & 0xCF) | (val & 0x30);
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
    else if (addr == 0xFF41) {
        // Bits 0-2 are read-only (mode + LYC flag), only bits 3-6 are writable
        io.stat = (io.stat & 0x07) | (val & 0x78);
    }
    else if (addr == 0xFF44) {
        return;  // LY is read-only
    }
    else if (addr == 0xFF46) {
        io.dma = val;
        dma_start(val);
    }
    // Sound registers
    else if (addr == 0xFF10) io.nr10 = val;
    else if (addr == 0xFF11) io.nr11 = val;
    else if (addr == 0xFF12) io.nr12 = val;
    else if (addr == 0xFF14) io.nr14 = val;
    else if (addr == 0xFF16) io.nr21 = val;
    else if (addr == 0xFF17) io.nr22 = val;
    else if (addr == 0xFF19) io.nr24 = val;
    else if (addr == 0xFF1A) io.nr30 = val;
    else if (addr == 0xFF1B) io.nr31 = val;
    else if (addr == 0xFF1C) io.nr32 = val;
    else if (addr == 0xFF1E) io.nr34 = val;
    else if (addr == 0xFF20) io.nr41 = val;
    else if (addr == 0xFF21) io.nr42 = val;
    else if (addr == 0xFF22) io.nr43 = val;
    else if (addr == 0xFF23) io.nr44 = val;
    else if (addr == 0xFF24) io.nr50 = val;
    else if (addr == 0xFF25) io.nr51 = val;
    else if (addr == 0xFF26) io.nr52 = val;
    // PPU registers
    else if (addr == 0xFF42) io.scy = val;
    else if (addr == 0xFF43) io.scx = val;
    else if (addr == 0xFF45) io.lyc = val;
    else if (addr == 0xFF47) io.bgp = val;
    else if (addr == 0xFF48) io.obp0 = val;
    else if (addr == 0xFF49) io.obp1 = val;
    else if (addr == 0xFF4A) io.wy = val;
    else if (addr == 0xFF4B) io.wx = val;
}