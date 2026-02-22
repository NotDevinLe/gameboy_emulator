#include "dma.h"
#include "ppu.h"
#include "bus.h"
#include <cstdio>
#include <cstdint>
#include <unistd.h>

struct dma_context {
    bool active;
    uint8_t byte;
    uint8_t value;
    uint8_t start_delay;
};

static dma_context ctx;

void dma_start(uint8_t start) {
    ctx.active = true;
    ctx.byte = 0;
    ctx.start_delay = 2;
    ctx.value = start;
}

void dma_tick() {
    if (!ctx.active) {
        return;
    }

    if (ctx.start_delay) {
        ctx.start_delay--;
        return;
    }

    ppu_oam_write(ctx.byte, bus_read((ctx.value * 0x100) + ctx.byte));

    ctx.byte++;

    ctx.active = ctx.byte < 0xA0;

    if (!ctx.active) {
        std::printf("DMA DONE!\n");
        sleep(2);
    }
}

bool dma_transferring() {
    return ctx.active;
}
