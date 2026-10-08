//
// Created by hatta on 2026/9/28.
//
#include<SDL3/SDL.h>
#include"engine_frame.h"
#include"engine_log.hpp"
#include"engine_window.hpp"
#include"lua_bind.hpp"
#include<filesystem>
#include<format>
namespace engine {

    bool Engine::Init() {
        using log = EngineLogger;
        SetCurrentCWDPath();
        auto fatal_handler = [this]() {
            OnDestroy();
        };
        if (!log::Init(fatal_handler)) {
            return false;
        }
        m_status = EngineStatus::Initializing;
        log::info("[engine]Engine Status: Initializing  引擎初始化开始");

        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
            log::err("[engine]  引擎初始化失败:SDL初始化失败,调用函数SDL_Init()时出错,错误信息:{}",SDL_GetError());
            return false;
        }
        log::info("[engine]  SDL初始化成功");

        if(!m_window.Init()) {
            log::err("[engine]  引擎初始化失败,创建窗口时失败,初始化已结束");
            return false;
        }
        log::info("[engine] SDL 窗口 创建成功");

        if (!m_renderer.Init(m_window.GetWindow())) {
            log::err("[engine]  引擎初始化失败,创建渲染器时失败,初始化已结束");
            return false;
        }
        log::info("[engine] SDL 渲染器 创建成功");

        if (!m_audio.Init()) {
            log::err("[engine]  引擎初始化失败，创建音频系统时失败");
            return false;
        }
        log::info("[engine]  音频系统初始化成功");

        if (!LuaWrapper::Init(m_lua) ) {
            log::err("[luajit]  luajit引擎初始化失败,初始化已结束");
            return false;
        }
        log::info("[luajit] luajit 引擎初始化完成");

        m_controller.Init();
        log::info("[engine] FrameController 初始化完成");
        m_status = EngineStatus::Running;
        log::info("[engine] 引擎初始化完成 Engine Status : Running");
        return true;
    }

    void Engine::EventHandle() {
        SDL_Event event;
        while (SDL_PollEvent(& event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    m_status = EngineStatus::OnDestroy ;
                    break;
            }
        }
    }
    void Engine::Destroy() {
        OnDestroy();
    }

    void Engine::OnUpdate() {
        GameUpdate();
    }
    void Engine::OnRender() {
        SDL_RenderClear(m_renderer.GetRenderer());
        GameRender();
        SDL_RenderPresent(m_renderer.GetRenderer());
    }
    void Engine::OnDestroy() {
        EngineLogger::info("[engine] 开始销毁资源");
        m_renderer.Destroy();
        m_window.Destroy();
        EngineLogger::info("[engine] 资源销毁结束 ，进程退出");
        exit(EXIT_FAILURE);
    }

    bool Engine::Run() {
        using log = EngineLogger;
        if (!LoadLuaScripts()) {
            log::err("[engine] 加载lua入口点脚本时发生错误，请检查 “packages/scripts/main.lua” 是否存在");
            return false;
        }
        log::info("[engine] 加载lua入口点脚本完成");
        GameInit();
        SDL_ShowWindow(m_window.GetWindow());
        while (m_status == EngineStatus::Running) {
            m_controller.BeginFrame();
            EventHandle();
            //处理事件
            m_controller.EndEvent();
            //处理逻辑
            OnUpdate();
            m_controller.EndUpdate();
            //渲染
            OnRender();
            m_controller.EndRender();
            //结束一次循环
            m_controller.EndFrame();
        }
        if (m_status == EngineStatus::OnDestroy) {
            OnDestroy();
        }

        return true;
    }

    bool Engine ::LoadLuaScripts() {
        using log = EngineLogger;
        // 获取根的绝对路径和相对路径
        const auto s = GetScriptsDir();
        const auto r = GetResourceDir();
        if (!s || !r) {
            return false;
        }
        std::string src = s.value();
        std::error_code ec;
        if (std::filesystem::create_directories(src,ec)) {
            log::info("[engine] 脚本路径 : “{}” 不存在,尝试创建...",src);
            if (ec) {
                log::err("[engine] 脚本路径 : “{}” 不存在,且创建时失败 : {} ",src ,ec.message());
                return false;
            }
            log::err("[engine] 脚本路径 : “{}” 已创建",src);
        }
        std::string res = r.value();
        if (std::filesystem::create_directories(res,ec)) {
            log::info("[engine] 资源路径 : “{}” 不存在,尝试创建...",res);
            if (ec) {
                log::err("[engine] 资源路径 : “{}” 不存在,且创建时失败 : {} ",res ,ec.message());
                return false;
            }
            log::err("[engine] 资源路径 : “{}” 已创建",res);
        }

        //将脚本路径追加到lua的搜索路径
        std::string old_path = m_lua["package"]["path"];
        m_lua["package"]["path"] =src+"?.lua;" + old_path;
        auto result = m_lua.safe_script_file(src +"main.lua",sol::script_pass_on_error);
        if (!result.valid()) {
            sol::error err = result;
            log::warn("[engine] 加载lua脚本时失败:{}",err.what());
            return false;
        }
        //收集入口定义的回调函数
        m_init = m_lua["GameInit"];
        if (!m_init.valid()) {
            log::err("[lua] 获取函数 GameInit 时失败");
            return false;
        }
        m_update = m_lua["GameUpdate"];
        if (!m_update.valid()) {
            log::err("[lua] 获取函数 GameUpdate 时失败");
            return false;
        }
        m_render = m_lua["GameRender"];
        if (!m_render.valid()) {
            log::err("[lua] 获取函数 GameRender 时失败");
            return false;
        }
        return true;
    }
    std::optional<std::string> Engine::GetScriptsRoot() {
        using log = EngineLogger;
        const char* base = SDL_GetBasePath();
        if (!base) {
            log::err("[engine] 获取脚本路径时失败,Error:{}",SDL_GetError());
            return std::nullopt;
        }
        std::string root = base ;
        SDL_free((void*)base);
        return root;
    }
    std::optional<std::string> Engine::GetScriptsDir() {
        return "packages/scripts/";
    }
    std::optional<std::string> Engine::GetResourceDir() {
        return "packages/resources/";
    }

    bool Engine::SetCurrentCWDPath() {
        auto r = GetScriptsRoot();
        if (!r) {
            return false;
        }
        auto cwd = std::filesystem::path(r.value());
        std::error_code ec;
        std::filesystem::current_path(cwd,ec);
        if (ec) {
            EngineLogger::err("[engine] 设置工作目录时出错: {}", ec.message());
            return false;
        }
        return true;
    }

    bool Engine::GameInit() {
        using log = EngineLogger;
        auto r = m_init();
        if (!r.valid()) {
            sol::error err = r;
            log::warn("[lua]  调用函数  GameInit 时失败 : {}",err.what());
            return false;
        }
        return true;
    }
    void Engine::GameUpdate() {
        auto r = m_update();
        if (!r.valid()) {
            sol::error err = r;
            EngineLogger::warn("[lua] 调用函数 GameUpdate 时失败 : {}",err.what());
        }
    }
    void Engine::GameRender() {
        auto r = m_render();
        if (!r.valid()) {
            sol::error err = r;
            EngineLogger::warn("[lua] 调用函数 GameRender时失败 : {}",err.what());
        }

    }
    //获取引擎实例
    Engine& Engine::GetInstance() {
        static Engine instance;
        return instance;
    }

    void Engine::SetFPS(Uint64 fps) noexcept {
        m_controller.SetFPS(fps);
    }

    void Engine::SetTitle(const char *title) noexcept {
        m_window.SetTitle(title);
    }

    void Engine::SetVsync(bool v) noexcept {
        m_window.SetVsync(v);
    }

    void Engine::SetWindowed(bool v) noexcept {
        m_window.SetWindowed(v);
        if (m_status == EngineStatus::Running)
            m_resized = true;

    }
    void Engine::SetResolution(Uint64 width, Uint64 height) noexcept {
        m_window.SetResolution(width, height);
        if (m_status == EngineStatus::Running)
            m_resized = true;
    }
    void Engine::LoadTexture(std::string_view path,std::string_view name) {
        m_renderer.LoadTexture(path,name);
    }
    void Engine:: LoadImage(std::string_view name , std::string_view tex , float x,float y ,float w,float h) {
        m_renderer.LoadImage(name,tex,x,y,w,h);
    }
    void Engine::DrawImage(std::string_view img , float x,float y,float rot ,float scale) {
        m_renderer.DrawImage(img,x,y,rot,scale);
    }
    void Engine::SetImageState(std::string_view img,BlendMode mode ,float r,float g,float b,float a) {
        m_renderer.SetImageState(img,mode ,r,g,b,a);
    }
    void Engine::SetImageScale(float scale) {
        m_renderer.SetImageScale(scale);
    }
    void Engine::SetImageCenter(std::string_view img ,float x,float y) {
        m_renderer.SetImageCenter(img,x,y);
    }
    void Engine::SetLogicalPresentMode(int w, int h) {
        m_renderer.SetLogicalPresentMode(w,h);
    }
    void Engine::LoadMusic(const std::string& name,std::string_view path ,Uint32 start,Uint32 end ,int loop) noexcept {
        m_audio.LoadMusic(name,path,start,end,loop);
    }
    void Engine::PlayMusic(std::string_view name) const noexcept {
        m_audio.PlayMusic(name);
    }
    void Engine::StopMusic(std::string_view name)const noexcept {
        m_audio.StopMusic(name);
    }
    void Engine::PauseMusic(std::string_view name)const noexcept {
        m_audio.PauseMusic(name);
    }
    void Engine::ResumeMusic(std::string_view name)const noexcept {
        m_audio.ResumeMusic(name);
    }
    void Engine::SetMusicVolume(float vol)const noexcept {
        m_audio.SetMusicVolume(vol);
    }
    void Engine::LoadSE(const std::string& name ,std::string_view path)noexcept {
        m_audio.LoadSE(name,path);
    }
    void Engine::PlaySE(std::string_view name)  noexcept {
        m_audio.PlaySE(name);
    }
    void Engine::SetSEVolume(float vol)const noexcept {
        m_audio.SetSEVolume(vol);
    }
}



