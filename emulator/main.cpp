#include <emu.h>
#include "cart.h"
#include "cpu.h"
#include <cstdio>
#include <csignal>
#include <cstring>
#include "ram.h"
#include "io.h"
#include "timer.h"
#include "ppu.h"
#include "ui.h"
#include "dma.h"
#include <iostream>

int main(int argc, char** argv) {

    // ensure that user provides a rom file
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <rom>" << std::endl;
        return 1;
    }

    // get the path
    const char* path = argv[1];

    // load the rom
    if (!cart_load(path)) {
        std::cout << "Failed to load ROM" << std::endl;
        return 1;
    }

    // initialize all subsystems
    cpu_init();
    ram_init();
    io_init();
    ppu_init();
    ui_init();

    // initialize the emulator context
    // and set the default values
    emu_context *ctx = emu_get_context();
    ctx->running = true;
    ctx->paused = false;
    ctx->die = false;

    // update frame every 70224 T-cycles
    static const uint32_t CYCLES_PER_FRAME = 70224;
    uint32_t frame_cycles = 0;

    uint8_t cycles = 0;
    while (ctx->running && !ctx->die) {
        // handle events
        ui_handle_events();

        // if paused, update the ui
        if (ctx->paused) {
            ui_update();
            continue;
        }

        // run cpu step
        cycles = cpu_step();

        // advance emulator cycles
        emu_cycles(cycles);

        // advance ppu
        ppu_step(cycles);

        // update ui once per frame
        frame_cycles += cycles;
        if (frame_cycles >= CYCLES_PER_FRAME) {
            frame_cycles -= CYCLES_PER_FRAME;
            ui_handle_events();
            ui_update();
        }
    }

    return 0;
}