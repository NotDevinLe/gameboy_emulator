#include "ui.h"
#include "emu.h"
#include "bus.h"
#include "io.h"
#include <cstdio>
#include <cstdint>

#include <SDL2/SDL.h>

// Basic setup of the UI screen
static const int SCALE = 4;
static const int SCREEN_W = 160;
static const int SCREEN_H = 144;
// Debugging screen size
static const int DBG_W = 16 * 8 * SCALE + 16 * SCALE;
static const int DBG_H = 24 * 8 * SCALE + 64 * SCALE;

// Main game window
static SDL_Window* gameWin;
static SDL_Renderer* gameRen;
static SDL_Texture* gameTex;
static SDL_Surface* gameSurf;

// Debugging window and texture
static SDL_Window* dbgWin;
static SDL_Renderer* dbgRen;
static SDL_Texture* dbgTex;
static SDL_Surface* dbgSurf;

// the gameboy has four colors: white, light gray, dark gray, and black
// each of these are associated codes in the gameboy's color palette
static uint32_t colors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};

void ui_init() {
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_Init(SDL_INIT_VIDEO);

    // Main game window (160x144 scaled up)
    SDL_CreateWindowAndRenderer(SCREEN_W * SCALE, SCREEN_H * SCALE, 0, &gameWin, &gameRen);
    SDL_SetWindowTitle(gameWin, "GameBoy");

    gameSurf = SDL_CreateRGBSurface(0, SCREEN_W, SCREEN_H, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    gameTex = SDL_CreateTexture(gameRen, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);

    // Debug tile viewer window
    SDL_CreateWindowAndRenderer(DBG_W, DBG_H, 0, &dbgWin, &dbgRen);
    SDL_SetWindowTitle(dbgWin, "VRAM Viewer");

    dbgSurf = SDL_CreateRGBSurface(0, DBG_W, DBG_H, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    dbgTex = SDL_CreateTexture(dbgRen, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, DBG_W, DBG_H);

    // Position debug window to the right of the game window
    int wx, wy;
    SDL_GetWindowPosition(gameWin, &wx, &wy);
    SDL_SetWindowPosition(dbgWin, wx + SCREEN_W * SCALE + 10, wy);
}

void delay(uint32_t ms) {
    SDL_Delay(ms);
}

// Apply the BGP palette register to a raw 2-bit color index.
// BGP (0xFF47) maps each color ID (0-3) to a shade:
//   bits 1-0 = color 0, bits 3-2 = color 1, bits 5-4 = color 2, bits 7-6 = color 3
static uint32_t palette_color(uint8_t color_id) {
    uint8_t shade = (io.bgp >> (color_id * 2)) & 0x03;
    return colors[shade];
}

// Draw a single tile into an SDL_Surface at pixel position (sx, sy).
// 'base' is the tile data start address (0x8000 or 0x8800).
// 'tile' is the tile index.
// 'scale' is the pixel scale factor (1 for game window, SCALE for debug).
// 'use_palette' applies BGP when true.
static void draw_tile(SDL_Surface* surf, uint16_t base, int tile, int sx, int sy,
                       int s, bool use_palette) {
    for (int row = 0; row < 8; row++) {
        uint8_t lo = bus_read(base + tile * 16 + row * 2);
        uint8_t hi = bus_read(base + tile * 16 + row * 2 + 1);
        for (int bit = 7; bit >= 0; bit--) {
            uint8_t px = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);
            uint32_t c = use_palette ? palette_color(px) : colors[px];
            SDL_Rect r = {sx + (7 - bit) * s, sy + row * s, s, s};
            SDL_FillRect(surf, &r, c);
        }
    }
}

// ---- Main game window: render the 160x144 background ----

static void update_game_window() {
    SDL_FillRect(gameSurf, nullptr, colors[0]);

    uint8_t lcdc = io.lcdc;

    // LCDC bit 0: BG enabled
    if (!(lcdc & 0x01)) goto present;

    {
        // LCDC bit 3: BG tile map area (0 = 0x9800, 1 = 0x9C00)
        uint16_t map_base = (lcdc & 0x08) ? 0x9C00 : 0x9800;

        // LCDC bit 4: BG & Window tile data area
        //   1 = 0x8000 (unsigned indexing: tile 0 = 0x8000)
        //   0 = 0x8800 (signed indexing:   tile 0 = 0x9000, tile -128 = 0x8800)
        bool unsigned_mode = (lcdc & 0x10) != 0;

        uint8_t scx = io.scx;
        uint8_t scy = io.scy;

        // For each pixel on the 160x144 screen, figure out which tile and which
        // pixel within that tile to draw.
        for (int ly = 0; ly < SCREEN_H; ly++) {
            uint8_t bg_y = static_cast<uint8_t>(scy + ly); // wraps at 256
            uint8_t tile_row = bg_y / 8;                    // which row of tiles (0-31)
            uint8_t pixel_row = bg_y % 8;                   // which row within the tile (0-7)

            for (int lx = 0; lx < SCREEN_W; lx++) {
                uint8_t bg_x = static_cast<uint8_t>(scx + lx); // wraps at 256
                uint8_t tile_col = bg_x / 8;                    // which column of tiles (0-31)
                uint8_t pixel_col = bg_x % 8;                   // which column within the tile (0-7)

                // Read tile index from the background map (32x32 grid)
                uint16_t map_addr = map_base + tile_row * 32 + tile_col;
                uint8_t tile_idx = bus_read(map_addr);

                // Calculate tile data address
                uint16_t tile_data_addr;
                if (unsigned_mode) {
                    // Tile 0 at 0x8000, tile 1 at 0x8010, etc.
                    tile_data_addr = 0x8000 + tile_idx * 16;
                } else {
                    // Signed mode: tile 0 at 0x9000, tile -128 at 0x8800
                    int8_t signed_idx = static_cast<int8_t>(tile_idx);
                    tile_data_addr = static_cast<uint16_t>(0x9000 + signed_idx * 16);
                }

                // Read the two bytes for this pixel's row within the tile
                uint8_t lo = bus_read(tile_data_addr + pixel_row * 2);
                uint8_t hi = bus_read(tile_data_addr + pixel_row * 2 + 1);

                // Extract the 2-bit color for this pixel column
                int shift = 7 - pixel_col;
                uint8_t color_id = ((hi >> shift) & 1) << 1 | ((lo >> shift) & 1);

                // Apply BGP palette and plot
                uint32_t c = palette_color(color_id);
                // gameSurf is 1:1 (160x144), scaling is handled by SDL_RenderCopy
                uint32_t* pixels = static_cast<uint32_t*>(gameSurf->pixels);
                pixels[ly * SCREEN_W + lx] = c;
            }
        }
    }

present:
    SDL_UpdateTexture(gameTex, nullptr, gameSurf->pixels, gameSurf->pitch);
    SDL_RenderClear(gameRen);
    SDL_RenderCopy(gameRen, gameTex, nullptr, nullptr);
    SDL_RenderPresent(gameRen);
}

// ---- Debug tile viewer window ----

static void update_dbg_window() {
    SDL_FillRect(dbgSurf, nullptr, 0xFF111111);

    int tile = 0, xd = 0, yd = 0;
    for (int y = 0; y < 24; y++) {
        for (int x = 0; x < 16; x++) {
            draw_tile(dbgSurf, 0x8000, tile++, xd + x * SCALE, yd + y * SCALE, SCALE, false);
            xd += 8 * SCALE;
        }
        yd += 8 * SCALE;
        xd = 0;
    }

    SDL_UpdateTexture(dbgTex, nullptr, dbgSurf->pixels, dbgSurf->pitch);
    SDL_RenderClear(dbgRen);
    SDL_RenderCopy(dbgRen, dbgTex, nullptr, nullptr);
    SDL_RenderPresent(dbgRen);
}

void ui_update() {
    update_game_window();
    update_dbg_window();
}

void ui_handle_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT ||
            (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE)) {
            emu_get_context()->die = true;
            return;
        }
    }
}
