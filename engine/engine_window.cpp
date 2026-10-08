//
// @author 帽子屋小姐
// @date 2026/9/29
//

#include "engine_window.hpp"
#include "engine_log.hpp"
#include <SDL3/SDL.h>
#include<format>
#include<string_view>
namespace engine {

    EngineWindow::EngineWindow() = default;
    EngineWindow :: ~EngineWindow() = default;
    bool EngineWindow::Init () {
        using log = EngineLogger;
        if (!m_width || !m_height) {
            m_width = 1280;
            m_height = 720;
        }
        if (m_window =SDL_CreateWindow(m_title.c_str(),m_width,m_height,
            SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN); !m_window ) {
            log::err("[engine] 创建调用函数 SDL_CreateWindow函数时失败:{}",SDL_GetError());
            return false;
        }
        return true;
    }

    void EngineWindow::SetResolution(int width, int height) {
        if (width <=0 || height <= 0) {
            m_width = 1280;
            m_height = 720;
            return;
        }
        m_width = width;
        m_height = height;
        if (!SDL_SetWindowSize(m_window,m_width,m_height)) {
            EngineLogger::err("[engine] 设置引擎窗口大小时出错 : {}",SDL_GetError());
        }
        if (!SDL_SetWindowPosition(m_window,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED)) {
            EngineLogger::err("[engine] 调用函数SDL_SetWindowPosition 时失败 :{}",SDL_GetError());
            return;
        }
    }

    void EngineWindow::SetWindowed(bool v) {
        m_windowed = v;
    }
    void EngineWindow::SetVsync(bool v) {
        m_vsync = v;
    }
    void EngineWindow::SetTitle(const char* title) {
        m_title = title;
        if (!SDL_SetWindowTitle(m_window,title)) {
            EngineLogger::err("[engine] 调用函数 SetTitle时失败 : {}",SDL_GetError());
        }
    }

    SDL_Window* EngineWindow::GetWindow() const noexcept {
        return m_window;
    }

    void EngineWindow::Destroy() {
        SDL_DestroyWindow(m_window);
        EngineLogger::info("[engine] SDL_Window 销毁");
    }
}
