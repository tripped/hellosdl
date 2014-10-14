#include <iostream>

#include "sdl.h"
#include "scopeguard.h"

const int SCREEN_W = 920;
const int SCREEN_H = 480;


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

    // Make texture from surface
    auto tex = ren.texture_from_surface(bmp);

    ren.clear();
    ren.copy(tex, NULL, NULL);
    ren.present();
    
    SDL_Delay(2000);

    return 0;
}
