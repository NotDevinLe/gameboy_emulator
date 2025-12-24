#include <emu.h>
#include "cart.h"
#include "cpu.h"
#include <cstdio>

int main(int argc, char** argv) {
    const char* path = (argc >= 2) ? argv[1] : "roms/pokemon_red.gb";

    if (!cart_load(path)) {
        std::printf("Failed to load ROM\n");
        return 1;
    }

    cpu_init();

    while (cpu_step()) {
        // later you will advance timers/PPU based on cycles
    }

    return 0;
}