#include "ppu.h"
#include "io.h"
#include "bus.h"
#include "ram.h"
#include "interrupt.h"
#include <cstdint>
#include <vector>
#include <stdlib.h>
#include <algorithm>
#include <iterator>

// some useful constants for the ppu
static const int SCREEN_WIDTH = 160;
static const int SCREEN_HEIGHT = 144;

static const uint32_t dmg_colors[4] = {
    0xFFFFFFFF,
    0xFFAAAAAA,
    0xFF555555,
    0xFF000000
};

static int ppu_dots = 0;
static uint8_t ppu_mode = 2;
static bool stat_irq_line = false;
static int window_line = 0;

uint32_t screen[SCREEN_WIDTH * SCREEN_HEIGHT];

// holds the 10 sprites allowed per scanline
Sprite sprites[10];
int found = 0;

static void set_mode(uint8_t mode);
static void check_lyc();
static void update_stat_irq();
static void get_sprites();
static void render_scanline();

static uint32_t palette_lookup(uint8_t palette, uint8_t color_index) {
    uint8_t shade = (palette >> (color_index * 2)) & 0x03;
    return dmg_colors[shade];
}

void ppu_init() {
    ppu_dots = 0;
    ppu_mode = 2;
    stat_irq_line = false;
    window_line = 0;
    io.ly = 0;
    std::fill(std::begin(screen), std::end(screen), dmg_colors[0]);
}

void ppu_step(uint8_t cycles) {

    // do a power check to see if the gameboy is powered on
    if (!(io.lcdc & 0x80)) {
        // turn off the ppu
        ppu_mode = 0;
        set_mode(0);
        io.ly = 0;
        ppu_dots = 0;
        stat_irq_line = false;
        window_line = 0;
        return;
    }

    // Do some update logic to see where we're actually at
    ppu_dots += cycles;

    while (1) {
        if (io.ly >= 144) {
            if (ppu_mode != 1) {
                set_mode(1);
                request_interrupt(0x01);
            }

            if (ppu_dots < 456) {
                break;
            }
            ppu_dots -= 456;
            io.ly++;
            if (io.ly > 153) {
                io.ly = 0;
                window_line = 0;
                set_mode(2);
                get_sprites();
            }
            check_lyc();
            continue;
        }

        if (ppu_mode == 2) {
            if (ppu_dots < 80) {
                break;
            }
            set_mode(3);
            render_scanline();
            continue;
        }

        if (ppu_mode == 3) {
            if (ppu_dots < 252) {
                break;
            }
            set_mode(0);
            continue;
        }

        if (ppu_mode == 0) {
            if (ppu_dots < 456) {
                break;
            }

            ppu_dots -= 456;
            io.ly++;
            check_lyc();
            if (io.ly >= 144) {
                continue;
            }
            set_mode(2);
            get_sprites();
        }
    }
}

void ppu_oam_write(uint16_t address, uint8_t value) {
    extern Ram ram;
    if (address >= 0xFE00) {
        address -= 0xFE00;
    }
    if (address < 0xA0) {
        ram.oam[address] = value;
    }
}

static void set_mode(uint8_t mode) {
    ppu_mode = mode;
    io.stat = (io.stat & 0xFC) | (mode & 0x03);
    update_stat_irq();
}

static void check_lyc() {
    if (io.ly == io.lyc) {
        io.stat |= 0x04;
    } else {
        io.stat &= ~0x04;
    }
    update_stat_irq();
}

static void update_stat_irq() {
    bool new_line = false;
    uint8_t mode = io.stat & 0x03;
    if ((mode == 0) && (io.stat & 0x08)) new_line = true;
    if ((mode == 1) && (io.stat & 0x10)) new_line = true;
    if ((mode == 2) && (io.stat & 0x20)) new_line = true;
    if ((io.stat & 0x04) && (io.stat & 0x40)) new_line = true;

    if (new_line && !stat_irq_line) {
        request_interrupt(0x02);
    }
    stat_irq_line = new_line;
}

static void get_sprites() {
    found = 0;
    for (int i = 0; i < 40; i++) {
        uint16_t addr = 0xFE00 + i * 0x4;
        int sprite_height = (io.lcdc & 0x04) ? 16 : 8;
        int y = static_cast<int>(bus_read(addr)) - 16;
        int x = static_cast<int>(bus_read(addr + 0x1)) - 8;
        uint8_t tile_id = bus_read(addr + 0x2);
        uint8_t attribute_flags = bus_read(addr + 0x3);

        if (io.ly >= y && io.ly < y + sprite_height) {
            sprites[found] = Sprite{x, y, tile_id, attribute_flags};
            found++;
        }
        if (found == 10) {
            break;
        }
    }

    std::stable_sort(sprites, sprites + found, [](const Sprite &a, const Sprite &b) {
        return a.x < b.x;
    });
}

static void render_scanline() {
    uint8_t addr_mode = (io.lcdc >> 4) & 1;
    uint8_t bg_tile_map = (io.lcdc >> 3) & 1;

    uint16_t base_addr = (bg_tile_map == 1) ? 0x9C00 : 0x9800;
    uint16_t tile_addr;

    uint8_t bg_scanline[SCREEN_WIDTH];
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        bg_scanline[i] = 0;
    }

    // background
    if (io.lcdc & 0x01) {
        for (int i = 0; i < SCREEN_WIDTH; i++) {
            int bg_x = (io.scx + i) & 0xFF;
            int bg_y = (io.scy + io.ly) & 0xFF;

            int tile_col = bg_x / 8;
            int tile_row = bg_y / 8;

            uint8_t tile_id = bus_read(base_addr + tile_row * 32 + tile_col);

            if (addr_mode == 1) {
                tile_addr = 0x8000 + tile_id * 16;
            } else {
                int8_t offset = static_cast<int8_t>(tile_id);
                tile_addr = 0x9000 + offset * 16;
            }

            int in_x = bg_x % 8;
            int in_y = bg_y % 8;

            uint16_t row_addr = tile_addr + 2 * in_y;
            uint8_t lo = bus_read(row_addr);
            uint8_t hi = bus_read(row_addr + 1);

            int bit = 7 - in_x;
            uint8_t color_index = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);

            screen[io.ly * SCREEN_WIDTH + i] = palette_lookup(io.bgp, color_index);
            bg_scanline[i] = color_index;
        }
    }

    // window
    if ((io.lcdc & 0x20) && io.ly >= io.wy) {
        int wx_start = io.wx - 7;
        uint16_t win_base = (io.lcdc & 0x40) ? 0x9C00 : 0x9800;
        bool window_visible = false;

        for (int i = 0; i < SCREEN_WIDTH; i++) {
            if (i < wx_start) continue;
            window_visible = true;

            int win_x = i - wx_start;
            int win_y = window_line;

            int tile_col = win_x / 8;
            int tile_row = win_y / 8;

            uint8_t tile_id = bus_read(win_base + tile_row * 32 + tile_col);

            if (addr_mode == 1) {
                tile_addr = 0x8000 + tile_id * 16;
            } else {
                int8_t offset = static_cast<int8_t>(tile_id);
                tile_addr = 0x9000 + offset * 16;
            }

            int in_x = win_x % 8;
            int in_y = win_y % 8;

            uint16_t row_addr = tile_addr + 2 * in_y;
            uint8_t lo = bus_read(row_addr);
            uint8_t hi = bus_read(row_addr + 1);

            int bit = 7 - in_x;
            uint8_t color_index = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);

            screen[io.ly * SCREEN_WIDTH + i] = palette_lookup(io.bgp, color_index);
            bg_scanline[i] = color_index;
        }

        if (window_visible) {
            window_line++;
        }
    }

    // sprites
    if (io.lcdc & 0x02) {
        // then we can draw the sprites previously found in mode 2
        // suppose you have indices (i < j) then we draw in reverse
        // so i is drawn over j (i has priority over j)
        for (int i = found - 1; i >= 0; i--) {
            Sprite sprite = sprites[i];
            int sprite_height = (io.lcdc & 0x04) ? 16 : 8;
            uint8_t tile = sprite.tile_id;
            int row = io.ly - sprite.y;

            bool priority = (sprite.attribute_flags >> 7) & 1;
            bool flip_y = (sprite.attribute_flags >> 6) & 1;
            bool flip_x = (sprite.attribute_flags >> 5) & 1;
            bool dmg_palette = (sprite.attribute_flags >> 4) & 1;

            if (flip_y) {
                row = sprite_height - 1 - row;
            }

            if (row < 0 || row >= sprite_height) {
                continue;
            }

            // in the case where this is a 8x16 pixel sprite
            // we want to either draw the top or bottom of the sprite
            int row_in_tile = row;
            if (sprite_height == 16) {
                uint8_t base = sprite.tile_id & 0xFE;
                if (row < 8) {
                    tile = base;
                } else {
                    tile = base + 1;
                    row_in_tile -= 8;
                }
            }

            uint16_t sprite_row_addr = 0x8000 + tile * 16 + row_in_tile * 2;
            uint8_t lo = bus_read(sprite_row_addr);
            uint8_t hi = bus_read(sprite_row_addr + 1);

            for (int j = 0; j < 8; j++) {
                int bit = 7 - j;

                if (flip_x) {
                    bit = 7 - bit;
                }

                uint8_t color_index = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);
                if (color_index == 0) {
                    continue;
                }

                uint8_t palette = dmg_palette ? io.obp1 : io.obp0;

                int sx = sprite.x + j;
                if (sx < 0 || sx >= SCREEN_WIDTH) {
                    continue;
                }

                int fb = io.ly * SCREEN_WIDTH + sx;

                if (priority && bg_scanline[sx] != 0) {
                    continue;
                }

                screen[fb] = palette_lookup(palette, color_index);
            }
        }
    }
}
