#include "ui.h"
#include "emu.h"
#include "bus.h"
#include "io.h"
#include "ram.h"
#include <cstdio>
#include <cstdint>
#include <algorithm>

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

// Apply a palette register to a 2-bit color index.
static uint32_t apply_palette(uint8_t palette, uint8_t color_id) {
    uint8_t shade = (palette >> (color_id * 2)) & 0x03;
    return colors[shade];
}

// Convenience wrapper for BGP
static uint32_t palette_color(uint8_t color_id) {
    return apply_palette(io.bgp, color_id);
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

// ---- Helpers for tile fetching ----

// Given a tile map address and LCDC bit 4 mode, return the address of the
// first byte of a tile's row data.
static uint16_t get_tile_data_addr(uint8_t tile_idx, bool unsigned_mode, uint8_t row) {
    uint16_t base;
    if (unsigned_mode) {
        base = 0x8000 + tile_idx * 16;
    } else {
        int8_t signed_idx = static_cast<int8_t>(tile_idx);
        base = static_cast<uint16_t>(0x9000 + signed_idx * 16);
    }
    return base + row * 2;
}

// Fetch the 2-bit color id for a pixel within a tile row.
static uint8_t get_tile_pixel(uint16_t tile_row_addr, uint8_t pixel_col) {
    uint8_t lo = bus_read(tile_row_addr);
    uint8_t hi = bus_read(tile_row_addr + 1);
    int shift = 7 - pixel_col;
    return ((hi >> shift) & 1) << 1 | ((lo >> shift) & 1);
}

// ---- Main game window: render BG + Window + Sprites ----

static void update_game_window() {
    uint32_t* pixels = static_cast<uint32_t*>(gameSurf->pixels);
    uint8_t lcdc = io.lcdc;

    // Track BG color IDs for sprite priority (0 = transparent BG)
    uint8_t bg_color_ids[SCREEN_W * SCREEN_H];
    std::fill(bg_color_ids, bg_color_ids + SCREEN_W * SCREEN_H, 0);

    // Fill with BG color 0 (white)
    uint32_t bg0 = palette_color(0);
    std::fill(pixels, pixels + SCREEN_W * SCREEN_H, bg0);

    bool unsigned_mode = (lcdc & 0x10) != 0;

    // ---- 1. Background layer (LCDC bit 0) ----
    if (lcdc & 0x01) {
        uint16_t map_base = (lcdc & 0x08) ? 0x9C00 : 0x9800;
        uint8_t scx = io.scx;
        uint8_t scy = io.scy;

        for (int ly = 0; ly < SCREEN_H; ly++) {
            uint8_t bg_y = static_cast<uint8_t>(scy + ly);
            uint8_t tile_row = bg_y / 8;
            uint8_t pixel_row = bg_y % 8;

            for (int lx = 0; lx < SCREEN_W; lx++) {
                uint8_t bg_x = static_cast<uint8_t>(scx + lx);
                uint8_t tile_col = bg_x / 8;
                uint8_t pixel_col = bg_x % 8;

                uint8_t tile_idx = bus_read(map_base + tile_row * 32 + tile_col);
                uint16_t addr = get_tile_data_addr(tile_idx, unsigned_mode, pixel_row);
                uint8_t cid = get_tile_pixel(addr, pixel_col);

                bg_color_ids[ly * SCREEN_W + lx] = cid;
                pixels[ly * SCREEN_W + lx] = palette_color(cid);
            }
        }
    }

    // ---- 2. Window layer (LCDC bit 5) ----
    if ((lcdc & 0x20) && (lcdc & 0x01)) {
        uint16_t win_map = (lcdc & 0x40) ? 0x9C00 : 0x9800;
        int wx = io.wx - 7;  // Window X is offset by 7
        int wy = io.wy;

        if (wy < SCREEN_H) {
            int win_line = 0;
            for (int ly = wy; ly < SCREEN_H; ly++) {
                uint8_t tile_row = win_line / 8;
                uint8_t pixel_row = win_line % 8;

                for (int lx = 0; lx < SCREEN_W; lx++) {
                    int screen_x = wx + lx;
                    if (screen_x < 0 || screen_x >= SCREEN_W) continue;

                    uint8_t tile_col = lx / 8;
                    uint8_t pixel_col = lx % 8;

                    uint8_t tile_idx = bus_read(win_map + tile_row * 32 + tile_col);
                    uint16_t addr = get_tile_data_addr(tile_idx, unsigned_mode, pixel_row);
                    uint8_t cid = get_tile_pixel(addr, pixel_col);

                    bg_color_ids[ly * SCREEN_W + screen_x] = cid;
                    pixels[ly * SCREEN_W + screen_x] = palette_color(cid);
                }
                win_line++;
            }
        }
    }

    // ---- 3. Sprites / OAM (LCDC bit 1) ----
    if (lcdc & 0x02) {
        bool tall_sprites = (lcdc & 0x04) != 0;  // 8x16 mode
        int sprite_h = tall_sprites ? 16 : 8;

        // Collect and sort sprites by X coordinate (lower X = higher priority).
        // On DMG, when X is equal, lower OAM index wins (already in order).
        struct SpriteEntry {
            int oam_idx;
            int x, y;
            uint8_t tile, flags;
        };
        SpriteEntry sprites[40];
        int sprite_count = 0;

        for (int i = 0; i < 40; i++) {
            int y = ram.oam[i * 4 + 0] - 16;  // screen Y
            int x = ram.oam[i * 4 + 1] - 8;   // screen X
            uint8_t tile = ram.oam[i * 4 + 2];
            uint8_t flags = ram.oam[i * 4 + 3];

            // Skip offscreen sprites
            if (x <= -8 || x >= SCREEN_W || y <= -sprite_h || y >= SCREEN_H)
                continue;

            sprites[sprite_count++] = {i, x, y, tile, flags};
        }

        // Draw sprites in reverse order so lower OAM index (higher priority) draws last
        for (int si = sprite_count - 1; si >= 0; si--) {
            auto& s = sprites[si];

            bool x_flip = (s.flags & 0x20) != 0;
            bool y_flip = (s.flags & 0x40) != 0;
            bool bg_priority = (s.flags & 0x80) != 0;
            uint8_t palette = (s.flags & 0x10) ? io.obp1 : io.obp0;

            uint8_t tile_num = s.tile;
            if (tall_sprites) tile_num &= 0xFE;  // ignore bit 0 for 8x16

            for (int row = 0; row < sprite_h; row++) {
                int screen_y = s.y + row;
                if (screen_y < 0 || screen_y >= SCREEN_H) continue;

                int tile_row = y_flip ? (sprite_h - 1 - row) : row;

                // For 8x16: top tile = tile_num, bottom tile = tile_num + 1
                uint8_t actual_tile = tile_num;
                if (tall_sprites) {
                    actual_tile = tile_num + (tile_row / 8);
                    tile_row %= 8;
                }

                // Sprites always use unsigned mode from 0x8000
                uint16_t addr = 0x8000 + actual_tile * 16 + tile_row * 2;
                uint8_t lo = bus_read(addr);
                uint8_t hi = bus_read(addr + 1);

                for (int col = 0; col < 8; col++) {
                    int screen_x = s.x + col;
                    if (screen_x < 0 || screen_x >= SCREEN_W) continue;

                    int bit = x_flip ? col : (7 - col);
                    uint8_t cid = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);

                    // Color 0 is transparent for sprites
                    if (cid == 0) continue;

                    // BG priority: sprite only shows over BG color 0
                    if (bg_priority && bg_color_ids[screen_y * SCREEN_W + screen_x] != 0)
                        continue;

                    pixels[screen_y * SCREEN_W + screen_x] = apply_palette(palette, cid);
                }
            }
        }
    }

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

// Map an SDL key to a Game Boy button. Returns -1 if unmapped.
static int key_to_btn(SDL_Keycode key) {
    switch (key) {
        case SDLK_RIGHT:     return BTN_RIGHT;
        case SDLK_LEFT:      return BTN_LEFT;
        case SDLK_UP:        return BTN_UP;
        case SDLK_DOWN:      return BTN_DOWN;
        case SDLK_z:         return BTN_A;
        case SDLK_x:         return BTN_B;
        case SDLK_BACKSPACE: return BTN_SELECT;
        case SDLK_RETURN:    return BTN_START;
        default:             return -1;
    }
}

void ui_handle_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT ||
            (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE)) {
            emu_get_context()->die = true;
            return;
        }
        if (e.type == SDL_KEYDOWN) {
            int btn = key_to_btn(e.key.keysym.sym);
            if (btn >= 0) joypad_press(static_cast<joypad_btn>(btn));
        }
        if (e.type == SDL_KEYUP) {
            int btn = key_to_btn(e.key.keysym.sym);
            if (btn >= 0) joypad_release(static_cast<joypad_btn>(btn));
        }
    }
}
