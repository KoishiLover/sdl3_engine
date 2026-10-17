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
#include"resource/TextRender.hpp"
#include"input/Input.hpp"
namespace engine {

    //引擎状态
    enum class EngineStatus {
        UnInitialized,
        Initializing,
        Running,
        OnExit,     // 正常退出（Lua 调用 GameExit）
        OnDestroy,  // 异常退出（窗口关闭 / 初始化失败）
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
        TextRender m_text;
        Audio  m_audio;
        FrameController m_controller ;
        Input m_input;
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
        ///@brief 清理资源（音频/渲染器/窗口）
        void ReleaseResources();
        ///@brief 正常退出进程
        void OnExit();
        ///@brief 异常退出进程
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
        [[nodiscard]]float GetFPS()const noexcept;
        //请求退出游戏：状态置为 OnExit，帧循环结束后清理资源并正常退出
        void GameExit() noexcept;
        //渲染接口
        void LoadTexture(std::string_view path,std::string_view name);
        void LoadImage(std::string_view name , std::string_view tex , float x,float y ,float w,float h);
        void DrawImage(std::string_view img , float x,float y,float rot ,float scale);
        void SetImageState(std::string_view img,BlendMode mode ,float r,float g,float b,float a);
        void SetImageScale(float scale);
        void SetImageCenter(std::string_view img ,float x,float y);
        void SetLogicalPresentMode(int w, int h);
        //世界坐标渲染：注册世界边界 / 切换当前世界
        void AddWorld(int id, int left, int right, int top, int bottom);
        void SetWorldID(int id);
        //文本接口
        void LoadTTF(std::string_view name, std::string_view path, float ptsize);
        void DrawText(std::string_view ttf, std::string_view text, int l, int r, int t, int b);
        void SetTextSize(float size, std::string_view ttf);
        void SetTextColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
        void SetTextAlign(TextRender::HAlign h, TextRender::VAlign v);
        //文本世界坐标：和图像的世界表相互独立
        void AddTextWorld(int id, int left, int right, int top, int bottom);
        void SetTextWorldID(int id);
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

        //处理输入
        [[nodiscard]]bool GetKeyState(int key)const noexcept;
        [[nodiscard]]bool IsKeyDown(int key)const noexcept;

        bool GameInit();
        void GameUpdate();
        void GameRender();
    };

}

#define IENGINE engine::Engine::GetInstance()

#endif //SDL3_ENGINE_ENGINE_FRAME_H
