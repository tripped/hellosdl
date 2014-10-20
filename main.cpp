#include <iostream>
#include <memory>
#include <cmath>

#include "sdl.h"
#include "distort.h"
#include "vec2.h"
#include "scopeguard.h"

const int SCREEN_W = 920;
const int SCREEN_H = 480;
const int TILE_SIZE = 256;
const double PI = 3.14159265;

typedef sdl::renderer::texture texture;


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
    auto porky = ren.texture_from_file("porky.png");
    double right = SCREEN_W - porky.width();
    double bottom = SCREEN_H - porky.height();

    vec2 position(right/2, bottom/2);
    vec2 velocity = random(2.0, 4.0) * vec2(random(0.0, 2*PI));

    // Create background distortion effect
    distortion dist(sdl::surface("bg.png"), ren, 2, 16.0, 0.1, 0.1, 1.0);

    bool done = false;
    SDL_Event e;
    while (!done) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                done = true;
            } else if (e.type == SDL_KEYDOWN) {
                auto k = e.key.keysym.sym;
                if (k == SDLK_SPACE) {
                    dist.type() = (dist.type() + 1) % 3;
                } else if (k == SDLK_UP) {
                    dist.amplitude() += 1;
                } else if (k == SDLK_DOWN) {
                    dist.amplitude() -= 1;
                } else if (k == SDLK_RIGHT) {
                    dist.frequency() += 0.001;
                } else if (k == SDLK_LEFT) {
                    dist.frequency() -= 0.001;
                } else if (k == SDLK_KP_MULTIPLY) {
                    dist.timescale() += 0.001;
                } else if (k == SDLK_KP_DIVIDE) {
                    dist.timescale() -= 0.001;
                } else if (k == SDLK_KP_PLUS) {
                    dist.compression() += 0.1;
                } else if (k == SDLK_KP_MINUS) {
                    dist.compression() -= 0.1;
                }
            }
        }

        dist.update();

        // update position
        position += velocity;

        // In case of collision, reflect vector around normal
        if (position.x < 0 || position.x > right ||
            position.y < 0 || position.y > bottom)
        {
            auto N = normal((position.x < 0), (position.y < 0),
                            (position.x > right), (position.y > bottom));
            auto U = velocity.dot(N) * N;
            auto W = velocity - U;
            velocity = W - U;

            // Apply random velocity and angle noise
            velocity.length(random(2.0, 4.0));
            velocity.angle(velocity.angle() + random(-0.7, 0.7));

            // Clamp position to avoid jitter
            position.x = std::max(0.0, std::min(right, position.x));
            position.y = std::max(0.0, std::min(bottom, position.y));
        }

        dist.render(0, 0, SCREEN_W, SCREEN_H);
        ren.copy(porky, position.x, position.y, porky.width()*2, porky.height()*2);

        ren.present();
    }

    return 0;
}
