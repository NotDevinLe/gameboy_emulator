#include <cpu.h>
#include <stack.h>
#include <interrupt.h>

void int_handle(cpu_state *ctx, uint16_t address) {
    stack_push16(ctx->PC);
    ctx->PC = address;
}

bool int_check(cpu_state *ctx, uint16_t address, interrupt_type it) {
    // Check if both IF and IE have this interrupt bit set
    if ((ctx->IF & it) && (ctx->IE & it)) {
        // If halted, wake up the CPU (even if IME is disabled)
        if (ctx->halt) {
            ctx->halt = false;
            // If IME is disabled, don't service the interrupt, just wake up
            if (!ctx->ime) {
                return false;
            }
        }
        
        // Interrupts can only be serviced if IME is enabled
        if (!ctx->ime) {
            return false;
        }
        
        // Service the interrupt
        int_handle(ctx, address);
        ctx->IF &= static_cast<uint8_t>(~it);  // Clear the interrupt flag
        ctx->halt = false;  // Exit halt state
        ctx->ime = false;   // Disable interrupts (will be re-enabled by RETI)

        return true;
    }

    return false;
}

void cpu_handle_interrupts(cpu_state *ctx) {
    // Check interrupts in priority order (VBLANK has highest priority)
    if (int_check(ctx, 0x40, IT_VBLANK)) {
        return;
    } else if (int_check(ctx, 0x48, IT_LCD_STAT)) {
        return;
    } else if (int_check(ctx, 0x50, IT_TIMER)) {
        return;
    } else if (int_check(ctx, 0x58, IT_SERIAL)) {
        return;
    } else if (int_check(ctx, 0x60, IT_JOYPAD)) {
        return;
    }
}

// Get the IF register value
uint8_t interrupt_get_if() {
    return cpu.IF;
}

// Set the IF register value
void interrupt_set_if(uint8_t value) {
    cpu.IF = value;
}

// Request an interrupt by setting the appropriate bit in IF
void interrupt_request(interrupt_type it) {
    cpu.IF |= it;
}

// Alias for interrupt_request (for compatibility)
void cpu_request_interrupt(interrupt_type t) {
    interrupt_request(t);
}
