#include <emu.h>
#include "cart.h"
#include "cpu.h"
#include <cstdio>
#include "ram.h"
#include "io.h"
#include "timer.h"
#include "io.h"
#include "ppu.h"

int main(int argc, char** argv) {
    const char* path = (argc >= 2) ? argv[1] : "roms/pokemon_red.gb";

    if (!cart_load(path)) {
        std::printf("Failed to load ROM\n");
        return 1;
    }

    cpu_init();
    ram_init();
    io_init();

    uint8_t cycles = 0;
    while (1) {
        cycles = cpu_step();
        for (int i = 0; i < cycles / 4; i++) {
            timer_tick();
        }
    } 
    ppu_step(cycles);
    return 0;
}