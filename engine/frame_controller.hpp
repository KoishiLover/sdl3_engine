//
// @author 帽子屋小姐
// @date 2026/9/30
//

#ifndef SDL3_ENGINE_FRAME_CONTROLLER_HPP
#define SDL3_ENGINE_FRAME_CONTROLLER_HPP
#include<SDL3/SDL_time.h>
namespace engine {

    class FrameController {
    public:
        FrameController() noexcept;
        FrameController(const FrameController&) = delete;
        FrameController& operator=(const FrameController&) = delete;

    //接口
    public:
        void SetFPS(uint32_t fps) noexcept;
        [[nodiscard]]Uint64 GetTargetFPS()const noexcept;
        [[nodiscard]]double  GetFPS()const noexcept;
        [[nodiscard]]double GetDeltaTime()const noexcept;
        void Begin() noexcept;
        void EndEvent() noexcept;
        void EndUpdate() noexcept;
        void EndRender()noexcept;

    private:
        Uint64 m_target_fps ;
        Uint64 m_start_time;
        Uint64 m_end_time;
        Uint64 m_event_duration;
        Uint64 m_update_duration ;
        Uint64 m_render_duration ;
        float m_delta_time;
        double m_target_frame_time;
    };
}

#endif //SDL3_ENGINE_FRAME_CONTROLLER_HPP
