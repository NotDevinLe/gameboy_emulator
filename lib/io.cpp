#include "io.h"
#include "interrupt.h"
#include "cpu.h"
#include "timer.h"
#include "emu.h"
#include <cstdio>
#include <cstdint>

// Serial port registers
static uint8_t sb_reg = 0; // Serial transfer data (SB, 0xFF01)
static uint8_t sc_reg = 0; // Serial control (SC, 0xFF02)

uint8_t io_read(uint16_t addr) {
    switch (addr) {
        case 0xFF00: // JOYPAD
            return 0x80;  // SameBoy returns 0x80 for blargg tests
            
        case 0xFF01: // SB - Serial transfer data
            return sb_reg;
            
        case 0xFF02: // SC - Serial control
            return sc_reg;
            
        case 0xFF04: // DIV
        case 0xFF05: // TIMA
        case 0xFF06: // TMA
        case 0xFF07: // TAC
            return timer_read(addr);
            
        case 0xFF0F: // IF - Interrupt flags
            return interrupt_get_if();
            
        case 0xFF44: // LY - Current scanline
            // Calculate LY based on machine cycles
            // Each scanline = 456 CPU cycles = 1824 machine cycles
            {
                emu_context *emu_ctx = emu_get_context();
                uint64_t machine_cycles = emu_ctx->ticks;
                
                constexpr uint64_t machine_cycles_per_scanline = 1824;  // 456 CPU cycles * 4
                constexpr uint8_t initial_ly = 0x80;  // LY starts at 0x80 after boot
                
                // Calculate scanlines passed
                uint64_t scanlines_passed = machine_cycles / machine_cycles_per_scanline;
                
                // LY = initial + scanlines_passed, wrapped to 0-153
                uint8_t ly = static_cast<uint8_t>((initial_ly + scanlines_passed) % 154);
                
                return ly;
            }
            
        default:
            return 0xFF;  // Unimplemented registers read as 0xFF
    }
}

void io_write(uint16_t addr, uint8_t value) {
    switch (addr) {
        case 0xFF01: // SB - Serial transfer data
            sb_reg = value;
            break;
            
        case 0xFF02: // SC - Serial control
            sc_reg = value;
            // When bit 7 (start) and bit 0 (internal clock) are set,
            // blargg's tests expect immediate serial transfer
            if ((sc_reg & 0x81) == 0x81) {
                std::putchar(static_cast<char>(sb_reg));
                std::fflush(stdout);
                
                // Request serial interrupt
                interrupt_request(IT_SERIAL);
                
                // Clear start bit (transfer complete)
                sc_reg &= static_cast<uint8_t>(~0x80u);
            }
            break;
            
        case 0xFF04: // DIV
        case 0xFF05: // TIMA
        case 0xFF06: // TMA
        case 0xFF07: // TAC
            timer_write(addr, value);
            break;
            
        case 0xFF0F: // IF - Interrupt flags
            interrupt_set_if(value);
            break;
            
        default:
            // Ignore writes to unimplemented registers
            break;
    }
}
