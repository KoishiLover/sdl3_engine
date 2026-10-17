//
// @author 帽子屋小姐
// @date 2026/10/3
//
#include"lua_bind.hpp"
#include"engine_frame.h"
#include"engine_log.hpp"
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

    float LuaWrapper::LW_GetFPS() noexcept {
        return IENGINE.GetFPS();
    }

    void LuaWrapper::LW_GameExit() noexcept {
        IENGINE.GameExit();
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
    void LuaWrapper::LW_AddWorld(int id, int left, int right, int top, int bottom) {
        IENGINE.AddWorld(id,left,right,top,bottom);
    }
    void LuaWrapper::LW_SetWorldID(int id) {
        IENGINE.SetWorldID(id);
    }
    ////文本相关
    void LuaWrapper::LW_LoadTTF(std::string_view name, std::string_view path, float ptsize) {
        IENGINE.LoadTTF(name,path,ptsize);
    }
    void LuaWrapper::LW_DrawText(std::string_view ttf, std::string_view text, int l, int r, int t, int b) {
        IENGINE.DrawText(ttf,text,l,r,t,b);
    }
    void LuaWrapper::LW_SetTextSize(float size, std::string_view ttf) {
        IENGINE.SetTextSize(size,ttf);
    }
    void LuaWrapper::LW_SetTextColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
        IENGINE.SetTextColor(r,g,b,a);
    }
    void LuaWrapper::LW_SetTextAlign(TextRender::HAlign h, TextRender::VAlign v) {
        IENGINE.SetTextAlign(h,v);
    }
    void LuaWrapper::LW_AddTextWorld(int id, int left, int right, int top, int bottom) {
        IENGINE.AddTextWorld(id,left,right,top,bottom);
    }
    void LuaWrapper::LW_SetTextWorldID(int id) {
        IENGINE.SetTextWorldID(id);
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

    bool LuaWrapper::LW_GetKeyState(int key) noexcept {
        return IENGINE.GetKeyState(key);
    }
    bool LuaWrapper::LW_IsKeyDown(int key) noexcept {
        return IENGINE.IsKeyDown(key);
    }

    ////日志相关
    //消息是作为「参数」交给 std::format 的，不是格式串 —— 脚本里写 {} %d 都不会被解析
    void LuaWrapper::LW_LogInfo(std::string_view message) {
        EngineLogger::info("[lua] {}",message);
    }
    void LuaWrapper::LW_LogWarn(std::string_view message) {
        EngineLogger::warn("[lua] {}",message);
    }
    void LuaWrapper::LW_LogErr(std::string_view message) {
        EngineLogger::err("[lua] {}",message);      //致命：弹消息框 + 退出进程
    }

}

namespace engine {
    bool LuaWrapper::Init(sol::state &m_lua) {
        m_lua.open_libraries(
                sol::lib::base, sol::lib::package, sol::lib::coroutine,
                sol::lib::string, sol::lib::table, sol::lib::math,
                sol::lib::io, sol::lib::os,
                sol::lib::jit, sol::lib::ffi );
        sol::table engine = m_lua.create_table("hatta");
        engine.set_function("SetFPS" ,&LuaWrapper::LW_SetFPS);
        engine.set_function("SetTitle" ,&LuaWrapper::LW_SetTitle);
        engine.set_function("SetVsync" ,&LuaWrapper::LW_SetVsync);
        engine.set_function("SetResolution" ,&LuaWrapper::LW_SetResolution);
        engine.set_function("SetWindowed" ,&LuaWrapper::LW_SetWindowed);
        engine.set_function("GetFPS" ,&LuaWrapper::LW_GetFPS);
        engine.set_function("GameExit" ,&LuaWrapper::LW_GameExit);
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
        engine.set_function("AddWorld",               &LuaWrapper::LW_AddWorld);
        engine.set_function("SetWorldID",             &LuaWrapper::LW_SetWorldID);
        //文本相关
        engine.new_enum("HAlign",
            "Left",   TextRender::HAlign::Left,
            "Center", TextRender::HAlign::Center,
            "Right",  TextRender::HAlign::Right);
        engine.new_enum("VAlign",
            "Top",    TextRender::VAlign::Top,
            "Middle", TextRender::VAlign::Middle,
            "Bottom", TextRender::VAlign::Bottom);
        engine.set_function("LoadTTF",        &LuaWrapper::LW_LoadTTF);
        engine.set_function("DrawText",       &LuaWrapper::LW_DrawText);
        engine.set_function("SetTextSize",    &LuaWrapper::LW_SetTextSize);
        engine.set_function("SetTextColor",   &LuaWrapper::LW_SetTextColor);
        engine.set_function("SetTextAlign",   &LuaWrapper::LW_SetTextAlign);
        engine.set_function("AddTextWorld",   &LuaWrapper::LW_AddTextWorld);
        engine.set_function("SetTextWorldID", &LuaWrapper::LW_SetTextWorldID);
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
        //键盘输入
        engine.set_function("GetKeyState",        &LuaWrapper::LW_GetKeyState);
        engine.set_function("IsKeyDown",          &LuaWrapper::LW_IsKeyDown);
        //日志
        engine.set_function("LogInfo",            &LuaWrapper::LW_LogInfo);
        engine.set_function("LogWarn",            &LuaWrapper::LW_LogWarn);
        engine.set_function("LogErr",             &LuaWrapper::LW_LogErr);

        //对象系统绑定（Object.New / GameObject usertype / 混合 __index/__newindex）
        GameObjectBind(m_lua);

        return m_lua["jit"].valid();
    }
}