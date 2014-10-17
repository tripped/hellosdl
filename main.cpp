#include <iostream>
#include <memory>
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


/*
 * Compute a specific frame of a distortion animation from
 * a source surface and write it into a destination surface.
 * (These should not be the same surface.)
 *
 * @param src   source surface
 * @param dst   destination surface (must have same size and format)
 * @param type  0=horz, 1=horz interlaced, 2=vert compression
 * @param t     tick number
 * @param A     amplitude of distortion wave
 * @param F     frequency
 * @param S     time scaling factor
 * @param C     compression factor, for vertical effect
 */
int distort_frame(SDL_Surface* src, SDL_Surface* dst, int type, int t,
        double A=16.0, double F=0.1, double S=0.1, double C=1.0)
{
    // Note that we're assuming 32-bit pixels here;
    // this may change for other formats
    int32_t* src_data = (int32_t*)src->pixels;
    int32_t* dst_data = (int32_t*)dst->pixels;

    for (int y = 0; y < src->h; ++y) {
        int offset = A * sinf(F * y + S * t);

        int new_x = 0;
        int new_y = y;

        if (type == 0) {
            new_x = offset;
        } else if (type == 1) {
            new_x = (y % 2)? offset : -offset;
        } else if (type == 2) {
            new_y = (int)(y * C + offset + src->h) % src->h;
        }

        for (int x = 0; x < src->w; ++x) {
            // Correctly wrap x offset
            new_x = (new_x + src->w) % src->w;
            dst_data[y*dst->w + x] = src_data[new_y*src->w + new_x];
            ++new_x;
        }
    }
    return 0;
}


/*
 * Class: distortion
 *
 * A basic battle background supporting just one wave function.
 * No palette transformations yet.
 */
class distortion
{
    sdl::surface src_;
    std::unique_ptr<sdl::surface> dst_;

    int type_;
    double amplitude_;
    double frequency_;
    double timescale_;
    double compression_;
    int ticks_;

public:
    enum type {
        horizontal,
        interlaced,
        compression
    };

    // TODO: always moving the bg argument is a little awkward,
    // try to come up with a better interface
    distortion(sdl::surface && bg, int type,
            double A, double F, double S, double C)
        : src_(std::move(bg)), type_(type), amplitude_(A),
        frequency_(F), timescale_(S), compression_(C), ticks_(0)
    {
        // Allocate a destination surface the same size as source
        // TODO: don't do this, just accept a target tex in render
        SDL_Surface* copy = SDL_CreateRGBSurface(0,
                src_.width(), src_.height(), 32,
                0x000000ff,
                0x0000ff00,
                0x00ff0000,
                0xff000000);
        dst_.reset(new sdl::surface(copy));
    }

    void update() {
        distort_frame(src_.handle(), dst_->handle(),
              type_, ticks_, amplitude_, frequency_, timescale_,
              compression_);
        ++ticks_;
    }

    void render(sdl::renderer & renderer, int x, int y, int w, int h) {
        // Do the stupid thing and create a texture right here
        auto tex = renderer.texture_from_surface(*dst_);
        renderer.copy(tex, x, y, w, h);
    }

    void render(sdl::renderer & renderer, int x, int y) {
        render(renderer, x, y, src_.width(), src_.height());
    }
};


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
    distortion dist(sdl::surface("bg.png"), 2, 16.0, 0.1, 0.1, 1.0);

    bool done = false;
    SDL_Event e;
    while (!done) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                done = true;
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

        dist.render(ren, 0, 0, SCREEN_W, SCREEN_H);
        ren.copy(porky, position.x, position.y, porky.width()*2, porky.height()*2);

        ren.present();
    }

    return 0;
}
