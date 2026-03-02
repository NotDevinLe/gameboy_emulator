#include "emu.h"
#include "timer.h"
#include "dma.h"
#include <cstdint>

static emu_context ctx = {};

emu_context *emu_get_context() {
    return &ctx;
}

void emu_cycles(int t_cycles) {
    for (int i = 0; i < t_cycles; i++) {
        ctx.ticks++;
        timer_tick();

        // every four ticks, run dma
        if ((ctx.ticks & 3) == 0) {
            dma_tick();
        }
    }
}