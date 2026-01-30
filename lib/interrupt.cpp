#include <cpu.h>
#include "stack.h"
#include "interrupt.h"
#include "bus.h"

uint8_t cpu_handle_interrupts(cpu_state *cpu) {
    // 1. Read the registers
    uint8_t ie = bus_read(0xFFFF);
    uint8_t if_reg = bus_read(0xFF0F);

    // 2. Handle HALT Waking (The "Alarm Clock" Effect)
    // Even if IME is disabled, an interrupt signal wakes the CPU.
    if (cpu->halt) {
        // If ANY interrupt is requested AND enabled in IE
        if (ie & if_reg) {
            cpu->halt = false;
        }
    }

    // 3. Service Interrupts (The "Context Switch")
    // This only happens if the Master Switch (IME) is ON.
    if (cpu->ime && (ie & if_reg)) {
        
        // Loop through bits 0-4 (Priority order is 0=Highest)
        // 0: VBlank, 1: LCD, 2: Timer, 3: Serial, 4: Joypad
        for (int i = 0; i < 5; i++) {
            uint8_t mask = 1 << i;
            
            // Check if THIS specific interrupt is requested AND enabled
            if ((ie & mask) && (if_reg & mask)) {
                
                // --- EXECUTE INTERRUPT ---
                
                // A. Disable Master Interrupts
                cpu->ime = false;
                
                // B. Clear the specific bit in IF
                // We read IF again just to be safe, modify bit, and write back.
                if_reg &= ~mask;
                bus_write(0xFF0F, if_reg);
                
                // C. Push current PC to Stack
                // NOTE: We push the address of the NEXT instruction
                stack_push16(cpu->PC);
                
                // D. Jump to the Vector Address
                // 0x40, 0x48, 0x50, 0x58, 0x60
                cpu->PC = 0x0040 + (i * 8);
                
                // E. Consume Cycles (Interrupts take 5 M-Cycles / 20 T-Cycles)
                return 20; 
            }
        }
    }

    return 0;
}