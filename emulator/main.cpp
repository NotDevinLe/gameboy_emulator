#include <emu.h>
#include "cart.h"
#include "cpu.h"
#include <cstdio>
#include "ram.h"

int main(int argc, char** argv) {
    const char* path = (argc >= 2) ? argv[1] : "roms/pokemon_red.gb";

    if (!cart_load(path)) {
        std::printf("Failed to load ROM\n");
        return 1;
    }

    cpu_init();
    ram_init();

    int step_count = 0;
    while (cpu_step()) {
        // step_count++;
        // if (step_count > 100) {
        //     break;
        // }
    }

    return 0;
}