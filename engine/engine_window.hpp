//
// @author 帽子屋小姐
// @date 2026/9/29
//

#ifndef SDL3_ENGINE_ENGINE_RENDER_HPP
#define SDL3_ENGINE_ENGINE_RENDER_HPP
#include <memory>
#include<string>
#include<SDL3/SDL_render.h>

namespace engine {

    class EngineWindow {
    public:
        EngineWindow();
        ~EngineWindow();
        void SetResolution(int width, int height);
        void SetWindowed(bool v);
        void SetVsync(bool v);
        void SetTitle(const char* title);
        bool Init();
        void Destroy();
    private:
        SDL_Window* m_window = nullptr ;
        int m_width ;
        int m_height ;
        bool m_vsync;
        bool m_windowed;
        std::string m_title;
    public:
        //给引擎调用，以获取 window;
        [[nodiscard]]SDL_Window* GetWindow()const noexcept;
    };

}

#endif //SDL3_ENGINE_ENGINE_RENDER_HPP
