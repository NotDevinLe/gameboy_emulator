#pragma once
#include <cstdint>

typedef struct {
    bool paused;
    bool running;
    uint64_t ticks;  // Machine cycles (4 per CPU cycle)
} emu_context;

int emu_run(int argc, char** argv);

emu_context *emu_get_context();

// Advance CPU cycles (for accurate timing)
// Each CPU cycle = 4 machine cycles, timer ticks once per machine cycle
void emu_cycles(int cpu_cycles);