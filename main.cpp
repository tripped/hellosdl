#include <iostream>
#include <cmath>

#include "sdl.h"
#include "vec2.h"
#include "scopeguard.h"

const int SCREEN_W = 920;
const int SCREEN_H = 480;
const int TILE_SIZE = 256;
const double PI = 3.14159265;

typedef sdl::renderer::texture texture;

/*
 * Function: render_scene
 *
 * Render a tiled background with a sprite at a given location.
 */
void render_scene(sdl::renderer& renderer,
        texture const& bg, texture const& sprite, int x, int y)
{
    int xtiles = SCREEN_W / TILE_SIZE + 1;
    int ytiles = SCREEN_H / TILE_SIZE + 1;

    for (int ty = 0; ty < ytiles; ++ty) {
        for (int tx = 0; tx < xtiles; ++tx) {
            renderer.copy(bg, tx*TILE_SIZE, ty*TILE_SIZE, TILE_SIZE, TILE_SIZE);
        }
    }

    renderer.copy(sprite, x, y);
}


/*
 * Function: random
 *
 * Return a normally-distributed random double between given bounds.
 */
double random(double from = 0.0, double to = 1.0)
{
    return from + to * ((double)rand() / (double)RAND_MAX);
}


/*
* Function: normal
*
* Find nearest edge or corner normal according to which edges
* of a bounding box have been exceeded (from within).
*/
vec2 normal(bool left, bool top, bool right, bool bottom) {
    double N;
    if      (top && left)       { N = -PI/4;    }
    else if (top && right)      { N = -3*PI/4;  }
    else if (bottom && left)    { N = PI/4;     }
    else if (bottom && right)   { N = 3*PI/4;   }
    else if (top)               { N = -PI/2;    }
    else if (bottom)            { N = PI/2;     }
    else if (left)              { N = 0;        }
    else if (right)             { N = PI;       }
    else {
        throw std::runtime_error("nonsense collision");
    }
    return vec2(N);
}


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

    // Load textures
    auto bg = ren.texture_from_file("bg.png");
    auto porky = ren.texture_from_file("porky.png");
    double right = SCREEN_W - porky.width();
    double bottom = SCREEN_H - porky.height();

    vec2 position(right/2, bottom/2);
    vec2 velocity = random(2.0, 4.0) * vec2(random(0.0, 2*PI));

    bool done = false;
    SDL_Event e;
    while (!done) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                done = true;
            }
        }

        // update position
        position += velocity;

        // In case of collision, reflect vector around normal
        if (position.x < 0 || position.x > right ||
            position.y < 0 || position.y > bottom)
        {
            auto N = normal((position.x < 0), (position.y < 0),
                            (position.x > right), (position.y > bottom));
            auto U = (velocity * N) * N;
            auto W = velocity - U;
            velocity = W - U;

            // Apply random velocity and angle noise
            velocity.length(random(2.0, 4.0));
            velocity.angle(velocity.angle() + random(-0.7, 0.7));

            // Clamp position to avoid jitter
            position.x = std::max(0.0, std::min(right, position.x));
            position.y = std::max(0.0, std::min(bottom, position.y));
        }

        render_scene(ren, bg, porky, position.x, position.y);
        ren.present();
    }

    return 0;
}
