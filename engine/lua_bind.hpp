//
// @author 帽子屋小姐
// @date 2026/10/2
//

#ifndef SDL3_ENGINE_LUA_BIND_HPP
#define SDL3_ENGINE_LUA_BIND_HPP

#include<SDL3/SDL_stdinc.h>
#include<sol/sol.hpp>
namespace engine {
    enum class BlendMode : Uint32;

    struct LuaWrapper {
        static void LW_SetFPS(Uint64 fps);
        static void LW_SetTitle(const char* title);
        static void LW_SetWindowed(bool v);
        static void LW_SetVsync(bool v);
        static void LW_SetResolution(int width, int height);
        static float LW_GetFPS()noexcept;
        static void LW_LoadTexture(std::string_view path,std::string_view name);
        static void LW_LoadImage(std::string_view name , std::string_view tex , float x,float y ,float w,float h);
        static void LW_DrawImage(std::string_view img , float x,float y,float rot ,float scale);
        static void LW_SetImageState(std::string_view img,BlendMode mode ,float r,float g,float b,float a);
        static void LW_SetImageScale(float scale);
        static void LW_SetImageCenter(std::string_view img ,float x,float y);
        static void LW_SetLogicalPresentMode(int w, int h);
        static void LW_LoadMusic(const std::string& name,std::string_view path ,Uint32 start,Uint32 end ,int loop) noexcept;
        static void LW_PlayMusic(std::string_view name)  noexcept;
        static void LW_StopMusic(std::string_view name) noexcept;
        static void LW_PauseMusic(std::string_view name) noexcept;
        static void LW_ResumeMusic(std::string_view name) noexcept;
        static void LW_SetMusicVolume(float vol) noexcept;
        static void LW_LoadSE(const std::string& name ,std::string_view path)noexcept;
        static void LW_PlaySE(std::string_view name)  noexcept;
        static void LW_SetSEVolume(float vol) noexcept;
        static bool LW_GetKeyState(int key)noexcept;
        static bool LW_IsKeyDown(int key)noexcept;
        [[nodiscard]] static bool Init(sol::state &m_lua );

    };
}

#endif //SDL3_ENGINE_LUA_BIND_HPP
