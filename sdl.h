#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>


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
        if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) {
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
        surface(std::string const& filename)
            : surface_(IMG_Load(filename.c_str())) { }
        surface(surface && other)
            : surface_(other.surface_) {
            other.surface_ = nullptr;
        }
        ~surface() {
            SDL_FreeSurface(this->surface_);
        }

        // Non-copyable for now
        surface(surface const& other) = delete;
        surface& operator=(surface const& rhs) = delete;

        int width() const {
            return surface_->w;
        }

        int height() const {
            return surface_->h;
        }

        SDL_Surface* handle() const {
            return this->surface_;
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
            int w_;
            int h_;
            friend class renderer;
        public:
            texture(renderer const& ren, std::string const& filename)
                : texture_(IMG_LoadTexture(ren.ren_, filename.c_str())) {
                if (this->texture_ == nullptr) {
                    throw error();
                }
                SDL_QueryTexture(texture_, NULL, NULL, &this->w_, &this->h_);
            }
            texture(renderer const& ren, surface const& sfc)
                : texture_(SDL_CreateTextureFromSurface(ren.ren_, sfc.surface_)) {
                if (this->texture_ == nullptr) {
                    throw error();
                }
                SDL_QueryTexture(texture_, NULL, NULL, &this->w_, &this->h_);
            }

            int width() const {
                return this->w_;
            }

            int height() const {
                return this->h_;
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

        texture texture_from_file(std::string const& filename) const {
            return texture(*this, filename);
        }

        texture texture_from_surface(surface const& sfc) const {
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

        void copy(texture const& tex, int x, int y, int w, int h) {
            SDL_Rect dst;
            dst.x = x;
            dst.y = y;
            dst.w = w;
            dst.h = h;
            this->copy(tex, NULL, &dst);
        }

        void copy(texture const& tex, int x, int y) {
            int w, h;
            SDL_QueryTexture(tex.texture_, NULL, NULL, &w, &h);
            this->copy(tex, x, y, w, h);
        }

        void present() const {
            SDL_RenderPresent(this->ren_);
        }
    };

}
