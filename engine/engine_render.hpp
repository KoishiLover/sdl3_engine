//
// @author 帽子屋小姐
// @date 2026/9/29
//

#ifndef SDL3_ENGINE_ENGINE_RENDER_HPP
#define SDL3_ENGINE_ENGINE_RENDER_HPP
#include <memory>
#include<SDL3/SDL_render.h>
#include<SDL3/SDL_stdinc.h>

namespace engine {
    struct SDLWindowDeleter {
        void operator()(SDL_Window* window)const noexcept {
            if (window) SDL_DestroyWindow(window);
        }
    };

    struct SDLRendererDeleter {
        void operator()(SDL_Renderer* renderer)const noexcept {
            if (renderer) SDL_DestroyRenderer(renderer);
        }
    };

    using EngineWindowPtr = std::unique_ptr<SDL_Window, SDLWindowDeleter>;
    using EngineRendererPtr = std::unique_ptr<SDL_Renderer, SDLRendererDeleter>;

    class EngineRenderer {
    public:
        EngineRenderer();
        ~EngineRenderer();
        void SetResolution(Uint64 width, Uint64 height);
        void SetWindowed(bool v);
        void SetVsync(bool v);
        void SetTitle(const char* title);
        bool Init();
    private:
        EngineWindowPtr window_ ;
        EngineRendererPtr renderer_ ;
        Uint64 m_width ;
        Uint64 m_height ;
        bool m_vsync;
        bool m_windowed;
        std::string m_title;
    };

}

#endif //SDL3_ENGINE_ENGINE_RENDER_HPP
