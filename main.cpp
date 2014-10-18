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
 * Struct: DISTORTION
 *
 * The parameters of an EarthBound-style distortion animation
 */
struct DISTORTION
{
    enum {
        HORIZONTAL,
        INTERLACED,
        VERTICAL
    };
    int type;
    double A;   // Amplitude
    double F;   // Frequency
    double S;   // Time scaling
    double C;   // Compression factor (VERTICAL only)
};

/*
 * Function template: distort_frame
 *
 * Compute a specific frame of an EarthBound-style distortion animation
 * from specified source pixels and write it into specified destination
 * pixels. These should not overlap. Pixels can be any integral type.
 *
 * @param src       Source pixels
 * @param src_pitch Length of a source row, in bytes
 * @param src_w     Width of the source, in pixels
 * @param src_h     Height of the source, in pixels
 * @param dst       Destination pixels
 * @param dst_pitch Length of a destination row, in bytes
 * @param params    Distortion parameters
 * @param t         Tick number of the frame to compute
 */
template<typename T>
int distort_frame(T* src, int src_pitch, int src_w, int src_h,
                  T* dst, int dst_pitch, DISTORTION const& params, int t)
{
    int const& type = params.type;
    double const& A = params.A;
    double const& F = params.F;
    double const& S = params.S;
    double const& C = params.C;

    for (int y = 0; y < src_h; ++y) {
        int offset = A * sinf(F * y + S * t);
        int new_x = 0;
        int new_y = y;

        if (type == DISTORTION::HORIZONTAL) {
            new_x = offset;
        } else if (type == DISTORTION::INTERLACED) {
            new_x = (y % 2)? offset : -offset;
        } else if (type == DISTORTION::VERTICAL) {
            new_y = (int)(y * C + offset + src_h) % src_h;
        }

        for (int x = 0; x < src_w; ++x) {
            // Correctly wrap x offset
            new_x = (new_x + src_w) % src_w;
            *((T*)((char*)(dst) + y*dst_pitch) + x) =
                *((T*)((char*)(src) + new_y*src_pitch) + new_x);
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
    sdl::renderer ren_;
    sdl::renderer::texture tex_;
    DISTORTION distort_;
    int ticks_;

public:
    enum type {
        horizontal,
        interlaced,
        compression
    };

    // TODO: always moving the bg argument is a little awkward,
    // try to come up with a better interface
    distortion(sdl::surface && bg, sdl::renderer const& renderer,
            int type, double A, double F, double S, double C)
        : src_(std::move(bg)),
          ren_(renderer),
          tex_(renderer, SDL_TEXTUREACCESS_STREAMING, src_.width(), src_.height()),
          distort_({type, A, F, S, C}),
          ticks_(0) { }

    void update() {
        auto lock = tex_.lock();

        distort_frame(src_.pixels(), src_.pitch(), src_.width(), src_.height(),
                lock.pixels, lock.pitch, distort_, ticks_);
        ++ticks_;
    }

    void render(int x, int y, int w, int h) {
        ren_.copy(tex_, x, y, w, h);
    }

    void render(int x, int y) {
        render(x, y, src_.width(), src_.height());
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
    distortion dist(sdl::surface("bg.png"), ren, 2, 16.0, 0.1, 0.1, 1.0);

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

        dist.render(0, 0, SCREEN_W, SCREEN_H);
        ren.copy(porky, position.x, position.y, porky.width()*2, porky.height()*2);

        ren.present();
    }

    return 0;
}
