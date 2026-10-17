//
// @author 帽子屋小姐
// @date 2026/10/8
//
#include<SDL3/SDL_scancode.h>
#include<SDL3/SDL_keyboard.h>
#include"Input.hpp"
#include"../engine_log.hpp"
namespace engine {
    using log = EngineLogger;
    bool Input::Init() {
        log::info("[engine]  初始化Input系统");
        m_keys = SDL_GetKeyboardState(&m_length);
        if (!m_keys) {
            log::err("[engine]  获取按键状态数组时失败 : {}",SDL_GetError());
            return false;
        }
        for (bool & m_last_key : m_last_keys) {
            m_last_key = false;
        }
        log::info("[engine]  初始化Input系统完成 keys数组长度 : {}",m_length);
        return true;
    }

    bool Input::GetKeyState(int keycode) const noexcept {
        return m_keys[keycode];
    }

    bool Input::IsKeyDown(int keycode) const noexcept {
        return (m_keys[keycode] && ! m_last_keys[keycode]) ;
    }

    void Input::UpdateLastInput() noexcept {
        for (size_t i = 0; i < m_length ; ++i) {
            m_last_keys[i] = m_keys[i];
        }
    }
}
