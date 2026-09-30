//
// Created by hatta on 2026/9/28.
//

#ifndef SDL3_ENGINE_ENGINE_FRAME_H
#define SDL3_ENGINE_ENGINE_FRAME_H
// include"engine_resource.hpp"
#include"engine_render.hpp"

#include<string>
namespace engine {

    //引擎状态
    enum class EngineStatus {
        UnInitialized,
        Running,
        OnDestroy,
    };

    class Engine {
    private:
        Engine() = default;
        ~Engine() = default;
    public:
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

    private:
        bool m_resized = false;
        EngineStatus m_status = EngineStatus::UnInitialized;
        EngineRenderer m_renderer ;


    public:
        bool Run();
        bool Init();
        static Engine& GetInstance();
    //内部调用
    private:
        void OnUpdate();
        void OnRender();
        void OnDestroy();
        bool LoadScripts();
    //接口方法
    public:
        void SetFPS(uint32_t fps) noexcept;
        void SetTitle(const char* title)noexcept;
        void SetWindowed(bool v)noexcept;
        void SetResolution(uint32_t width, uint32_t height)noexcept;
        void SetVsync(bool v)noexcept;
    };

}

#define IENGINE engine::Engine::GetInstance()

#endif //SDL3_ENGINE_ENGINE_FRAME_H
