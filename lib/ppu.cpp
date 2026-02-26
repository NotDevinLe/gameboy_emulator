#include "ppu.h"
#include "io.h"
#include "bus.h"
#include "ram.h"
#include <cstdint>

// PPU timing constants (in T-cycles / dots)
static const int OAM_SCAN_DOTS   = 80;   // Mode 2
static const int DRAWING_DOTS    = 172;   // Mode 3 (variable, but 172 is typical)
static const int SCANLINE_DOTS   = 456;   // Total dots per scanline
static const int VBLANK_START    = 144;   // First VBlank line
static const int LINES_PER_FRAME = 154;   // Total lines (0-153)

// PPU State
static int ppu_dots = 0;
static uint8_t ppu_mode = 2;  // Start in OAM scan

void ppu_init() {
    ppu_dots = 0;
    ppu_mode = 2;
    io.ly = 0;
}

// Set the PPU mode in the STAT register (bits 0-1)
static void set_mode(uint8_t mode) {
    ppu_mode = mode;
    io.stat = (io.stat & 0xFC) | (mode & 0x03);
}

// Check LYC=LY coincidence and update STAT bit 2, fire STAT interrupt if enabled
static void check_lyc() {
    if (io.ly == io.lyc) {
        io.stat |= 0x04;  // Set coincidence flag (bit 2)
        // If LYC=LY interrupt enabled (STAT bit 6), request STAT interrupt
        if (io.stat & 0x40) {
            uint8_t if_reg = bus_read(0xFF0F);
            bus_write(0xFF0F, if_reg | 0x02);  // STAT interrupt = bit 1
        }
    } else {
        io.stat &= ~0x04;  // Clear coincidence flag
    }
}

// Request a STAT interrupt if the corresponding mode's STAT enable bit is set
static void check_stat_interrupt(uint8_t mode) {
    bool fire = false;
    switch (mode) {
        case 0: fire = (io.stat & 0x08) != 0; break;  // Mode 0 (HBlank) interrupt
        case 1: fire = (io.stat & 0x10) != 0; break;  // Mode 1 (VBlank) interrupt
        case 2: fire = (io.stat & 0x20) != 0; break;  // Mode 2 (OAM) interrupt
    }
    if (fire) {
        uint8_t if_reg = bus_read(0xFF0F);
        bus_write(0xFF0F, if_reg | 0x02);  // STAT interrupt = bit 1
    }
}

void ppu_step(uint8_t cycles) {
    // If LCD is disabled, reset state
    if (!(io.lcdc & 0x80)) {
        ppu_dots = 0;
        io.ly = 0;
        set_mode(0);
        return;
    }

    // Advance by T-cycles (cpu_step returns M-cycles, main loop multiplies by 4... 
    // actually checking main.cpp: cycles = cpu_step() then ppu_step(cycles) 
    // and emu_cycles loops cpu_cycles times calling timer_tick which does +=4
    // So cycles here is M-cycles. Convert to T-cycles (dots).
    ppu_dots += cycles * 4;

    // Process complete scanline chunks
    while (ppu_dots >= SCANLINE_DOTS) {
        ppu_dots -= SCANLINE_DOTS;

        io.ly++;

        if (io.ly >= LINES_PER_FRAME) {
            io.ly = 0;
        }

        check_lyc();

        if (io.ly < VBLANK_START) {
            // Visible scanline — start with OAM scan (mode 2)
            set_mode(2);
            check_stat_interrupt(2);
        } else if (io.ly == VBLANK_START) {
            // Enter VBlank (mode 1)
            set_mode(1);
            // VBlank interrupt (IF bit 0)
            uint8_t if_reg = bus_read(0xFF0F);
            bus_write(0xFF0F, if_reg | 0x01);
            check_stat_interrupt(1);
        }
        // Lines 145-153: stay in mode 1 (VBlank)
    }

    // Within a visible scanline, cycle through modes based on dot position
    if (io.ly < VBLANK_START) {
        if (ppu_dots < OAM_SCAN_DOTS) {
            if (ppu_mode != 2) {
                set_mode(2);
                check_stat_interrupt(2);
            }
        } else if (ppu_dots < OAM_SCAN_DOTS + DRAWING_DOTS) {
            if (ppu_mode != 3) {
                set_mode(3);
                // No STAT interrupt for mode 3
            }
        } else {
            if (ppu_mode != 0) {
                set_mode(0);
                check_stat_interrupt(0);
            }
        }
    }
}

void ppu_oam_write(uint16_t address, uint8_t value) {
    extern Ram ram;
    if (address >= 0xFE00) {
        address -= 0xFE00;
    }
    if (address < 0xA0) {
        ram.oam[address] = value;
    }
}
