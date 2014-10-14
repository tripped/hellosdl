#include <iostream>

#include "sdl.h"
#include "scopeguard.h"

const int SCREEN_W = 920;
const int SCREEN_H = 480;
const int TILE_SIZE = 256;


int main(int argc, char** argv)
{
    sdl::init(SDL_INIT_EVERYTHING);

    // Always de-init SDL on the way out
    scopeguard quit = make_guard(sdl::quit);

    // Make a pretty window
    auto win = sdl::window(
            "Hello!", 100, 100, SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);

    // Create a rendering context
    auto ren = sdl::renderer(win, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Load surface
    auto bmp = sdl::bitmap("hello.bmp");

    auto bg = ren.texture_from_file("bg.png");
    auto porky = ren.texture_from_file("porky.png");

    int xtiles = SCREEN_W / TILE_SIZE + 1;
    int ytiles = SCREEN_H / TILE_SIZE + 1;

    ren.clear();
    for (int y = 0; y < ytiles; ++y) {
        for (int x = 0; x < xtiles; ++x) {
            ren.copy(bg, x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE);
        }
    }

    ren.copy(porky, SCREEN_W/2 - 50, SCREEN_H/2 - 50);

    ren.present();
    
    SDL_Delay(5000);

    return 0;
}
