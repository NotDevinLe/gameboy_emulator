#include "emu.h"
#include "timer.h"
#include "dma.h"
#include <cstdint>

static emu_context ctx = {};

emu_context *emu_get_context() {
    return &ctx;
}

void emu_cycles(int cpu_cycles) {
    // Each CPU cycle = 4 machine cycles
    // timer_tick() should be called every 4 machine cycles (once per CPU cycle)
    // This way timer.counter increments by 4 per CPU cycle
    // DIV increments every 256 machine cycles = 64 CPU cycles (correct!)
    for (int i = 0; i < cpu_cycles; i++) {
        ctx.ticks += 4;  // Track machine cycles
        timer_tick();    // Called once per CPU cycle, increments counter by 4
        dma_tick();      // Tick DMA (handles OAM transfers)
    }
}

int emu_run(int /*argc*/, char** /*argv*/) {
    // This is handled in main.cpp
    return 0;
}
