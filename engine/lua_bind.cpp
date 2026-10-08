//
// @author 帽子屋小姐
// @date 2026/10/3
//
#include"lua_bind.hpp"
#include"engine_frame.h"
namespace engine {
    void LuaWrapper::LW_SetFPS(Uint64 fps) {
        IENGINE.SetFPS(fps);
    }

    void LuaWrapper::LW_SetTitle(const char *title) {
        IENGINE.SetTitle(title);
    }

    void LuaWrapper::LW_SetVsync(bool v) {
        IENGINE.SetVsync(v);
    }

    void LuaWrapper::LW_SetResolution(int width, int height) {
        IENGINE.SetResolution(width, height);
    }

    void LuaWrapper::LW_SetWindowed(bool v) {
        IENGINE.SetWindowed(v);
    }
    ////渲染相关
    void LuaWrapper::LW_LoadTexture(std::string_view path,std::string_view name) {
        IENGINE.LoadTexture(path,name);
    }
    void LuaWrapper::LW_LoadImage(std::string_view name , std::string_view tex , float x,float y ,float w,float h) {
        IENGINE.LoadImage(name,tex ,x,y,w,h);
    }
    void LuaWrapper::LW_DrawImage(std::string_view img , float x,float y,float rot ,float scale) {
        IENGINE.DrawImage(img ,x,y,rot,scale);
    }
    void LuaWrapper::LW_SetImageState(std::string_view img,BlendMode mode ,float r,float g,float b,float a) {
        IENGINE.SetImageState(img ,mode ,r,g,b,a);
    }
    void LuaWrapper::LW_SetImageScale(float scale) {
        IENGINE.SetImageScale(scale);
    }
    void LuaWrapper::LW_SetImageCenter(std::string_view img ,float x,float y) {
        IENGINE.SetImageCenter(img ,x,y);
    }
    void LuaWrapper::LW_SetLogicalPresentMode(int w, int h) {
        IENGINE.SetLogicalPresentMode(w,h);
    }
    /////音频相关
    void LuaWrapper::LW_LoadMusic(const std::string& name,std::string_view path ,Uint32 start,Uint32 end ,int loop) noexcept {
        IENGINE.LoadMusic(name,path ,start,end,loop);
    }
    void LuaWrapper::LW_PlayMusic(std::string_view name)  noexcept {
        IENGINE.PlayMusic(name);
    }
    void LuaWrapper::LW_StopMusic(std::string_view name) noexcept {
        IENGINE.StopMusic(name);
    }
    void LuaWrapper:: LW_PauseMusic(std::string_view name) noexcept {
        IENGINE.PauseMusic(name);
    }
    void LuaWrapper:: LW_ResumeMusic(std::string_view name) noexcept {
        IENGINE.ResumeMusic(name);
    }
    void LuaWrapper:: LW_SetMusicVolume(float vol) noexcept {
        IENGINE.SetMusicVolume(vol);
    }
    void LuaWrapper::LW_LoadSE(const std::string& name ,std::string_view path)noexcept {
        IENGINE.LoadSE(name, path);
    }
    void LuaWrapper::LW_PlaySE(std::string_view name)  noexcept {
        IENGINE.PlaySE(name);
    }
    void LuaWrapper::LW_SetSEVolume(float vol) noexcept {
        IENGINE.SetSEVolume(vol);
    }
}

namespace engine {
    bool LuaWrapper::Init(sol::state &m_lua) {
        m_lua.open_libraries(sol::lib::base , sol::lib::jit,sol::lib::ffi , sol::lib::package );
        sol::table engine = m_lua.create_table("hatta");
        engine.set_function("SetFPS" ,&LuaWrapper::LW_SetFPS);
        engine.set_function("SetTitle" ,&LuaWrapper::LW_SetTitle);
        engine.set_function("SetVsync" ,&LuaWrapper::LW_SetVsync);
        engine.set_function("SetResolution" ,&LuaWrapper::LW_SetResolution);
        engine.set_function("SetWindowed" ,&LuaWrapper::LW_SetWindowed);
        //渲染相关
        engine.new_enum("BlendMode",
            "None",BlendMode::None,
            "Blend",BlendMode::Blend,
            "Add",BlendMode::Add,
            "Mod",BlendMode::Mod,
            "Mul",BlendMode::Mul);
        engine.set_function("LoadTexture",        &LuaWrapper::LW_LoadTexture);
        engine.set_function("LoadImage",          &LuaWrapper::LW_LoadImage);
        engine.set_function("DrawImage",          &LuaWrapper::LW_DrawImage);
        engine.set_function("SetImageState",      &LuaWrapper::LW_SetImageState);
        engine.set_function("SetImageScale",      &LuaWrapper::LW_SetImageScale);
        engine.set_function("SetImageCenter",     &LuaWrapper::LW_SetImageCenter);
        engine.set_function("SetLogicalPresentSize",  &LuaWrapper::LW_SetLogicalPresentMode);
        //音频相关
        engine.set_function("LoadMusic",          &LuaWrapper::LW_LoadMusic);
        engine.set_function("PlayMusic",          &LuaWrapper::LW_PlayMusic);
        engine.set_function("StopMusic",          &LuaWrapper::LW_StopMusic);
        engine.set_function("PauseMusic",         &LuaWrapper::LW_PauseMusic);
        engine.set_function("ResumeMusic",         &LuaWrapper::LW_ResumeMusic);
        engine.set_function("SetMusicVolume",      &LuaWrapper::LW_SetMusicVolume);
        engine.set_function("LoadSE",             &LuaWrapper::LW_LoadSE);
        engine.set_function("PlaySE",             &LuaWrapper::LW_PlaySE);
        engine.set_function("SetSEVolume",         &LuaWrapper::LW_SetSEVolume);

        return m_lua["jit"].valid();
    }
}