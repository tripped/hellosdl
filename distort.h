/*
 * Battle animation distortions
 */
#pragma once
#include "sdl.h"

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
 * Encapsulates distort parameters and state and manages rendering
 * with SDL.
 */
class distortion
{
    sdl::surface src_;
    sdl::renderer ren_;
    sdl::renderer::texture tex_;
    DISTORTION distort_;
    int ticks_;

public:
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

    int& type() {
        return distort_.type;
    }

    double& amplitude() {
        return distort_.A;
    }

    double& frequency() {
        return distort_.F;
    }

    double& timescale() {
        return distort_.S;
    }

    double& compression() {
        return distort_.C;
    }
};
