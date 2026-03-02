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
static const int OAM_SCAN_DOTS = 80;
static const int DRAWING_DOTS = 172;
static const int SCANLINE_DOTS = 456;
static const int VBLANK_START = 144;
static const int LINES_PER_FRAME = 154;
static const int SCREEN_WIDTH = 160;
static const int SCREEN_HEIGHT = 144;

// ppu state
static int ppu_dots = 0;
static uint8_t ppu_mode = 2;  // Start in OAM scan


// The screen itself
// Where screen[i] = (i % screen_width, i // screen_width)
uint32_t screen[SCREEN_WIDTH * SCREEN_HEIGHT];

// holds the 10 sprites allowed per scanline
Sprite sprites[10];
int found = 0;

// set the ppu mode
static void set_mode(uint8_t mode);

// check if LYC == LY
static void check_lyc();

// check if the STAT interrupt is enabled for the given mode
static void check_stat_interrupt(uint8_t mode);

static void get_sprites();

static void render_scanline();

void ppu_init() {
    ppu_dots = 0;
    ppu_mode = 2;
    // what scanline are we on
    io.ly = 0;
    std::fill(std::begin(screen), std::end(screen), 0xFFFF00FF);
}

void ppu_step(uint8_t cycles) {

    // do a power check to see if the gameboy is powered on
    if (!(io.lcdc & 0x80)) {
        // turn off the ppu
        ppu_mode = 0;
        set_mode(0);
        io.ly = 0;
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
                set_mode(2);
                get_sprites();
            }
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
            check_stat_interrupt(0);
            continue;
        }
        
        if (ppu_mode == 0) {
            // handle H-Blank
            if (ppu_dots < 456) {
                break;
            }

            ppu_dots -= 456;
            io.ly++;
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
    check_stat_interrupt(mode);
}

static void check_lyc() {
    if (io.ly == io.lyc) {
        // set the coincidence flag
        io.stat |= 0x04;

        // in the case that the lyc interrupt is enabled, request the interrupt
        if (io.stat & 0x40) {
            request_interrupt(0x02);
        }
    } else {
        // clear the coincidence flag
        io.stat &= ~0x04;
    }
}

static void check_stat_interrupt(uint8_t mode) {
    bool fire = false;
    switch (mode) {
        case 0: fire = (io.stat & 0x08) != 0; break;
        case 1: fire = (io.stat & 0x10) != 0; break;
        case 2: fire = (io.stat & 0x20) != 0; break;
    }
    if (fire) {
        request_interrupt(0x02);
    }
}

static void get_sprites() {
    // take care of the OAM mode.
    // for each sprite held in the OAM
    found = 0;
    for (int i = 0; i < 40; i++) {
        uint16_t addr = 0xFE00 + i * 0x4;
        int sprite_height = (io.lcdc & 0x04) ? 16 : 8;
        int y = static_cast<int>(bus_read(addr)) - sprite_height;
        int x = static_cast<int>(bus_read(addr + 0x1)) - 8;
        uint8_t tile_id = bus_read(addr + 0x2);
        uint8_t attribute_flags = bus_read(addr + 0x3);

        // check if we should draw this sprite
        // ie. its y position falls within the plausible range
        if (io.ly >= y && io.ly < y + sprite_height) {
            sprites[found] = Sprite{x, y, tile_id, attribute_flags};
            found++;
        }
        if (found == 10) {
            break;
        }
    }
}

static void render_scanline() {
    // this is the mode where we actually draw the pixels
    // First, we want to draw the background
    // We need to find which indexing type to use (bit 4 lcdc)

    uint8_t addr_mode = (io.lcdc >> 4) & 1;
    uint8_t bg_tile_map = (io.lcdc >> 3) & 1;

    uint16_t base_addr;
    uint16_t tile_addr;

    if (bg_tile_map == 1) {
        base_addr = 0x9C00;
    } else {
        base_addr = 0x9800;
    }

    // initialize current background with all 0
    uint8_t bg_scanline[SCREEN_WIDTH];
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        bg_scanline[i] = 0;
    }

    if (io.lcdc & 1) {
        // for each pixel in the current scanline
        for (int i = 0; i < 160; i++) {
            // The real background is actually 256x256 pixels
            // We introduce wrap-around logic to display BG
            int bg_x = (io.scx + i) & 0xFF;
            int bg_y = (io.scy + io.ly) & 0xFF;

            // This address is in terms of pixels but we want counts of tiles
            int tile_col = static_cast<int>(bg_x / 8);
            int tile_row = static_cast<int>(bg_y / 8);

            // Now we can get the tile ID from the VRAM
            uint8_t tile_id = bus_read(base_addr + tile_row * 32 + tile_col);

            // Get the location of the actual tile data
            if (addr_mode == 1) {
                tile_addr = 0x8000 + tile_id * 16;
            } else {
                int8_t offset = static_cast<int8_t>(tile_id);
                tile_addr = 0x9000 + offset * 16;
            }

            // within a tile, what is the pixel of that tile?
            int in_x = bg_x % 8;
            int in_y = bg_y % 8;

            // each pixel row is comprised of 8 bits (or 2 bytes)
            uint16_t row_addr = tile_addr + 2 * in_y;
            uint8_t lo = bus_read(row_addr);
            uint8_t hi = bus_read(row_addr + 1);

            int bit = 7 - in_x;
            uint8_t color_index = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);
            uint8_t palette = io.bgp;

            uint8_t color = ((palette) >> (color_index * 2)) & 0x3;
            screen[io.ly * SCREEN_WIDTH + i] = color;
            bg_scanline[i] = color_index;
        }
    }

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

            // Get the location of the actual tile data
            tile_addr = 0x8000 + sprite.tile_id * 16;
            
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

            // get the actual tile data
            uint16_t tile_addr = 0x8000 + tile * 16 + row_in_tile * 2;
            uint8_t lo = bus_read(tile_addr);
            uint8_t hi = bus_read(tile_addr + 1);

            for (int j = 0; j < 8; j++) {
                int bit = 7 - j;

                if (flip_x) {
                    bit = 7 - bit;
                }

                uint8_t color_index = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);
                if (color_index == 0) {
                    continue;
                }

                // get current palette
                uint8_t palette;
                if (dmg_palette) {
                    palette = io.obp1;
                } else {
                    palette = io.obp0;
                }

                int color = (palette >> (color_index * 2)) & 0x03;

                int sx = sprite.x + j;
                if (sx < 0 || sx >= SCREEN_WIDTH) {
                    continue;
                }

                int fb = io.ly * SCREEN_WIDTH + sx;

                if (priority) {
                    // this means we have to draw background over the sprites unless background is 0
                    if (bg_scanline[sx] == 0) {
                        screen[fb] = color;
                    }
                } else {
                    screen[fb] = color;
                }
            }
        }
    }
}