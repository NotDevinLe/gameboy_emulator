#pragma once
#include <cstdint>

typedef struct {
    bool paused;
    bool running;
    bool die;        // signal emulator should exit
    uint64_t ticks;  // machine cycles (4 per cpu cycle)
} emu_context;

emu_context *emu_get_context();

// advance cpu cycles
void emu_cycles(int cpu_cycles);