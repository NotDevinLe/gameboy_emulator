#include <cpu.h>
#include "stack.h"
#include "interrupt.h"
#include "bus.h"

uint8_t cpu_handle_interrupts(cpu_state *cpu) {
    uint8_t ie = bus_read(0xFFFF);
    uint8_t if_reg = bus_read(0xFF0F);

    if (cpu->halt) {
        if (ie & if_reg) {
            cpu->halt = false;
        }
    }

    if (cpu->ime && (ie & if_reg)) {
        
        // Loop through bits 0-4 (Priority order is 0=Highest)
        // 0: VBlank, 1: LCD, 2: Timer, 3: Serial, 4: Joypad
        for (int i = 0; i < 5; i++) {
            uint8_t mask = 1 << i;
            
            // check if ie flag is enabled
            if ((ie & mask) && (if_reg & mask)) {
                cpu->ime = false;

                if_reg &= ~mask;
                bus_write(0xFF0F, if_reg);
                stack_push16(cpu->PC);
                cpu->PC = 0x0040 + (i * 8);
                return 20; 
            }
        }
    }

    return 0;
}

void request_interrupt(uint8_t type) {
    uint8_t if_reg = bus_read(0xFF0F);
    if_reg |= type;
    bus_write(0xFF0F, if_reg);
    
    if (cpu.halt) {
        cpu.halt = false;
    }
}