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

static void signal_handler(int /*sig*/) {
    emu_get_context()->die = true;
}

int main(int argc, char** argv) {
    const char* path = nullptr;
    bool headless = false;

    // Parse args: [--headless] <rom>
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--headless") == 0) {
            headless = true;
        } else if (!path) {
            path = argv[i];
        }
    }
    if (!path) path = "roms/pokemon_red.gb";

    if (!cart_load(path)) {
        std::printf("Failed to load ROM\n");
        return 1;
    }

    // Initialize all subsystems
    cpu_init();
    ram_init();
    io_init();
    ppu_init();

    if (!headless) {
        ui_init();
    }

    // Install signal handler so Ctrl+C exits cleanly
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    emu_context *ctx = emu_get_context();
    ctx->running = true;
    ctx->paused = false;
    ctx->die = false;

    // Throttle UI: only update once per frame (~70224 T-cycles)
    static const uint32_t CYCLES_PER_FRAME = 70224;
    uint32_t frame_cycles = 0;

    uint8_t cycles = 0;
    while (ctx->running && !ctx->die) {
        if (!headless) {
            ui_handle_events();
        }

        if (ctx->paused) {
            if (!headless) ui_update();
            continue;
        }

        // Run CPU step
        cycles = cpu_step();

        // Advance emulator cycles (handles timer and DMA)
        emu_cycles(cycles);

        // Advance PPU
        ppu_step(cycles);

        // Only update UI once per frame, not every instruction
        if (!headless) {
            frame_cycles += cycles * 4; // T-cycles
            if (frame_cycles >= CYCLES_PER_FRAME) {
                frame_cycles -= CYCLES_PER_FRAME;
                ui_handle_events();
                ui_update();
            }
        }
    }

    return 0;
}