#include "ppu.h"
#include "io.h"      // Access to io.ly and io.lcdc
#include "bus.h"     // Access to interrupts (0xFF0F)
#include <stdint.h>

// PPU State
int ppu_dots = 0;

void ppu_init() {
    ppu_dots = 0;
}

void ppu_step(uint8_t cycles) {
    // 1. Check if LCD is Enabled (Bit 7 of LCDC)
    // If 0, PPU is off. LY is fixed at 0.
    if (!(io.lcdc & 0x80)) {
        ppu_dots = 0;
        io.ly = 0;
        // Optionally: set mode to 0 in STAT register
        return;
    }

    // 2. Advance Internal Counter
    ppu_dots += cycles;

    // 3. Handle Scanline Logic
    // A complete scanline is 456 dots (114 M-Cycles)
    while (ppu_dots >= 456) {
        ppu_dots -= 456;

        // Increment LY (Current Line)
        io.ly++;

        // 4. Handle V-Blank (Line 144)
        if (io.ly == 144) {
            // Trigger V-Blank Interrupt (Bit 0 of IF)
            uint8_t if_reg = bus_read(0xFF0F);
            bus_write(0xFF0F, if_reg | 0x01);
        }

        // 5. Handle Frame Reset (Line 154)
        // Lines 0-143 are visible. 144-153 are V-Blank.
        // After 153, it rolls back to 0.
        if (io.ly > 153) {
            io.ly = 0;
        }
    }
}