#include <iostream>
#include <SDL2/SDL.h>

#include "scopeguard.h"

const int SCREEN_W = 920;
const int SCREEN_H = 480;


namespace sdl {
    /*
     * Forward declarations
     */

    class window;
    class renderer;
    class surface;
    class bitmap;

    /*
     * class: error
     */
    class error : public std::exception
    {
    public:
        error() {
            this->msg_ = SDL_GetError();
        }
        const char* what() const throw() {
            return this->msg_;
        }
    private:
        const char* msg_;
    };


    void init(uint32_t flags) {
        if (SDL_Init(flags) != 0) {
            throw error();
        }
    }

    void quit() {
        SDL_Quit();
    }


    /*
     * class: window
     * Exception-safe, automatically managed SDL_CreateWindow.
     */
    class window
    {
        SDL_Window* win_;
        friend class renderer;
    public:
        window(std::string const& title, int x, int y, int w, int h,
                uint32_t flags) {
            this->win_ = SDL_CreateWindow(title.c_str(), x, y, w, h, flags);
            if (this->win_ == nullptr) {
                throw error();
            }
        }
        ~window() {
            SDL_DestroyWindow(this->win_);
        }
    };

    class surface
    {
    protected:
        SDL_Surface* surface_;
        friend class renderer;
    public:
        surface(SDL_Surface* s) : surface_(s) { }
        ~surface() {
            SDL_FreeSurface(this->surface_);
        }
    };

    class bitmap : public surface
    {
    public:
        bitmap(std::string const& filename)
                : surface(SDL_LoadBMP(filename.c_str())) {
            if (this->surface_ == nullptr) {
                throw error();
            }
        }
    };

    class renderer
    {
        SDL_Renderer* ren_;
    public:
        class texture
        {
            SDL_Texture* texture_;
            friend class renderer;
        public:
            texture(renderer const& ren, surface const& sfc)
                : texture_(SDL_CreateTextureFromSurface(ren.ren_, sfc.surface_)) {
                if (this->texture_ == nullptr) {
                    throw error();
                }
            }
        };

        renderer(window const& win, int index, uint32_t flags) {
            this->ren_ = SDL_CreateRenderer(win.win_, index, flags);
            if (this->ren_ == nullptr) {
                throw error();
            }
        }

        ~renderer() {
            SDL_DestroyRenderer(this->ren_);
        }

        texture texture_from_surface(surface const& sfc) {
            return texture(*this, sfc);
        }

        void clear() const {
            if (SDL_RenderClear(this->ren_) != 0) {
                throw error();
            }
        }

        void copy(texture const& tex, const SDL_Rect* src,
                const SDL_Rect* dst) const {
            if (SDL_RenderCopy(this->ren_, tex.texture_, src, dst) != 0) {
                throw error();
            }
        }

        void present() const {
            SDL_RenderPresent(this->ren_);
        }
    };

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
