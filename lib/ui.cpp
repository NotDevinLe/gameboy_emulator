#include "ui.h"
#include "emu.h"
#include "ppu.h"
#include "bus.h"
#include "io.h"
#include <cstdio>
#include <cstdint>
#include <cstring>

#include <SDL2/SDL.h>

// Basic setup of the UI screen
static const int SCALE = 4;
static const int SCREEN_W = 160;
static const int SCREEN_H = 144;

// Main game window
static SDL_Window* gameWin;
static SDL_Renderer* gameRen;
static SDL_Texture* gameTex;
static SDL_Surface* gameSurf;

void ui_init() {
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_Init(SDL_INIT_VIDEO);

    SDL_CreateWindowAndRenderer(SCREEN_W * SCALE, SCREEN_H * SCALE, 0, &gameWin, &gameRen);
    SDL_SetWindowTitle(gameWin, "GameBoy");

    gameSurf = SDL_CreateRGBSurface(0, SCREEN_W, SCREEN_H, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    gameTex = SDL_CreateTexture(gameRen, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);
}

void delay(uint32_t ms) {
    SDL_Delay(ms);
}

static void update_game_window() {
    std::memcpy(gameSurf->pixels, screen, SCREEN_W * SCREEN_H * sizeof(uint32_t));

    SDL_UpdateTexture(gameTex, nullptr, gameSurf->pixels, gameSurf->pitch);
    SDL_RenderClear(gameRen);
    SDL_RenderCopy(gameRen, gameTex, nullptr, nullptr);
    SDL_RenderPresent(gameRen);
}

void ui_update() {
    update_game_window();
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
