//
// Created by hatta on 2026/9/28.
//
#include"engine_frame.h"
#include"engine_log.hpp"
namespace engine {

    bool Engine::Init() {return true;}
    void Engine::OnUpdate() {}
    void Engine::OnRender() {}
    void Engine::OnDestroy() {}

    bool Engine::Run() {return true;}
    bool Engine::LoadScripts() {return true; }
    //获取引擎实例
    Engine& Engine::GetInstance() {
        static Engine instance;
        return instance;
    }

    void Engine::SetFPS(uint32_t fps) noexcept {

    }

    void Engine::SetTitle(const char *title) noexcept {
        m_renderer.SetTitle(title);
    }

    void Engine::SetVsync(bool v) noexcept {
        m_renderer.SetVsync(v);
    }

    void Engine::SetWindowed(bool v) noexcept {
        m_renderer.SetWindowed(v);
        if (m_status == EngineStatus::Running)
            m_resized = true;

    }

    void Engine::SetResolution(uint32_t width, uint32_t height) noexcept {
        m_renderer.SetResolution(width, height);
        if (m_status == EngineStatus::Running)
            m_resized = true;
    }
}



