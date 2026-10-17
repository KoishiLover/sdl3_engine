//
// @author 帽子屋小姐
// @date 2026/10/8
//

#ifndef HATTA_ENGINE_INPUT_HPP
#define HATTA_ENGINE_INPUT_HPP
#include<array>
#include<SDL3/SDL_keycode.h>
namespace engine {
    class Input {
    public:
        Input() = default;
        ~Input() = default;
        bool Init();
    private:
        int m_length;
        const bool* m_keys = nullptr;
        std::array<bool,512> m_last_keys;

    public:
        [[nodiscard]]bool GetKeyState(int keycode) const noexcept;
        [[nodiscard]]bool IsKeyDown(int keycode) const noexcept;
        void UpdateLastInput() noexcept;
    };
}
#endif //HATTA_ENGINE_INPUT_HPP
