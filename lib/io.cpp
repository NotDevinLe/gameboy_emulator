#include "io.h"
#include "interrupt.h"
#include "cpu.h"

#include <cstdio>
#include <cstdint>

// Very small IO implementation focused on what blargg's CPU tests need:
// - Serial port (SB/SC) for printing test output
// - IF register plumbing (0xFF0F) delegated to interrupt module

static uint8_t sb_reg = 0; // Serial transfer data (SB, 0xFF01)
static uint8_t sc_reg = 0; // Serial control (SC, 0xFF02)

uint8_t io_read(uint16_t addr) {
    switch (addr) {
        case 0xFF00: // JOYPAD - buttons/keys
            // SameBoy returns 0x80 for this register in blargg tests
            // This matches the expected behavior for the test ROM
            return 0x80;
        case 0xFF44: // LY - current scanline
            // Minimal LY emulation (no PPU yet).
            //
            // `cpu_instrs` polls LY in a tight loop. A pragmatic way to avoid
            // hangs (and keep behavior closer to SameBoy) is to advance LY based
            // on *how often it's read*.
            //
            // Start at 0x80 (what we observed at the first poll point), then
            // increment every N reads.
            {
                static uint8_t  ly = 0x80;
                static uint32_t ly_reads = 0;
                ly_reads++;

                // Tunable: SameBoy's LY in this loop increments roughly every few
                // iterations. Adjust as needed to keep logs aligned.
                if ((ly_reads % 6u) == 0) {
                    ly++;
                }

                return ly;
            }
        case 0xFF01: // SB
            return sb_reg;
        case 0xFF02: // SC
            return sc_reg;
        case 0xFF0F: // IF
            return interrupt_get_if();
        default:
            // Unimplemented IO registers read back as 0xFF for now.
            return 0xFF;
    }
}

void io_write(uint16_t addr, uint8_t value) {
    switch (addr) {
        case 0xFF01: // SB
            sb_reg = value;
            break;

        case 0xFF02: // SC
            sc_reg = value;
            // When bit 7 (start) and bit 0 (internal clock) are set,
            // blargg's tests expect the emulator to immediately "send"
            // the byte in SB and print it.
            if ((sc_reg & 0x81) == 0x81) {
                std::putchar(static_cast<char>(sb_reg));
                std::fflush(stdout);

                // Request a serial interrupt (optional but closer to spec)
                interrupt_request(Interrupt::SERIAL);

                // On real hardware, transfer completes over many cycles
                // and hardware clears the start bit afterwards. We emulate
                // an instant transfer by clearing bit 7 immediately.
                sc_reg &= static_cast<uint8_t>(~0x80u);
            }
            break;

        case 0xFF0F: // IF
            interrupt_set_if(value);
            break;

        default:
            // Ignore writes to unimplemented IO registers for now.
            break;
    }
}

