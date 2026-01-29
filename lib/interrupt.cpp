#include <cpu.h>
#include <stack.h>
#include <interrupt.h>

void int_handle(cpu_state *ctx, u16 address) {
    stack_push16(ctx->PC);
    ctx->PC = address;
}

bool int_check(cpu_state *ctx, u16 address, interrupt_type it) {
    if (ctx->IF & it && ctx->IE & it) {
        int_handle(ctx, address);
        ctx->IF &= ~it;
        ctx->halt = false;
        ctx->ime = false;

        return true;
    }

    return false;
}

void cpu_handle_interrupts(cpu_state *ctx) {
    if (int_check(ctx, 0x40, IT_VBLANK)) {

    } else if (int_check(ctx, 0x48, IT_LCD_STAT)) {

    } else if (int_check(ctx, 0x50, IT_TIMER)) {

    }  else if (int_check(ctx, 0x58, IT_SERIAL)) {

    }  else if (int_check(ctx, 0x60, IT_JOYPAD)) {

    } 
}
