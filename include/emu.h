#pragma once
#include <cstdint>

typedef struct {
    bool paused;
    bool running;
    uint16_t ticks;
} emu_context;

int emu_run(int argc, char** argv);

emu_context *emu_get_context();