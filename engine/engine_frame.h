//
// Created by hatta on 2026/9/28.
//

#ifndef SDL3_ENGINE_ENGINE_FRAME_H
#define SDL3_ENGINE_ENGINE_FRAME_H
#include"engine_window.hpp"
#include"frame_controller.hpp"
#include<optional>
#include<sol/state.hpp>
#include"resource/Renderer.hpp"
#include"resource/Audio.hpp"
namespace engine {

    //引擎状态
    enum class EngineStatus {
        UnInitialized,
        Initializing,
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
        EngineWindow m_window ;
        Renderer m_renderer;
        Audio  m_audio;
        FrameController m_controller ;

        sol::state m_lua;
        //lua侧的回调函数
        sol::protected_function m_init;
        sol::protected_function m_update;
        sol::protected_function m_render;

    public:
        bool Run();
        bool Init();
        void Destroy() ;
        static Engine& GetInstance();
    //内部调用
    private:
        ///@brief 处理事件
        void EventHandle();
        ///@brief 更新逻辑
        void OnUpdate();
        ///@brief 渲染
        void OnRender();
        ///@brief 退出进程
        void OnDestroy();
        ///@brief 加载Lua 脚本
        bool LoadLuaScripts();

        std::optional<std::string> GetScriptsRoot();
        std::optional<std::string> GetScriptsDir();
        std::optional<std::string> GetResourceDir();
        bool SetCurrentCWDPath();
    //接口方法
    public:
        //框架接口
        void SetFPS(Uint64 fps) noexcept;
        void SetTitle(const char* title)noexcept;
        void SetWindowed(bool v)noexcept;
        void SetResolution(Uint64 width, Uint64 height)noexcept;
        void SetVsync(bool v)noexcept;
        //渲染接口
        void LoadTexture(std::string_view path,std::string_view name);
        void LoadImage(std::string_view name , std::string_view tex , float x,float y ,float w,float h);
        void DrawImage(std::string_view img , float x,float y,float rot ,float scale);
        void SetImageState(std::string_view img,BlendMode mode ,float r,float g,float b,float a);
        void SetImageScale(float scale);
        void SetImageCenter(std::string_view img ,float x,float y);
        void SetLogicalPresentMode(int w, int h);
        //音频接口
        void LoadMusic(const std::string& name,std::string_view path ,Uint32 start,Uint32 end ,int loop) noexcept;
        void PlayMusic(std::string_view name) const noexcept;
        void StopMusic(std::string_view name)const noexcept;
        void PauseMusic(std::string_view name)const noexcept;
        void ResumeMusic(std::string_view name)const noexcept;
        void SetMusicVolume(float vol)const noexcept;
        void LoadSE(const std::string& name ,std::string_view path)noexcept;
        void PlaySE(std::string_view name)  noexcept;
        void SetSEVolume(float vol)const noexcept;


        bool GameInit();
        void GameUpdate();
        void GameRender();
    };

}

#define IENGINE engine::Engine::GetInstance()

#endif //SDL3_ENGINE_ENGINE_FRAME_H
