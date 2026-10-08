//
// @author 帽子屋小姐
// @date 2026/9/30
//
#include "frame_controller.hpp"
#include <SDL3/SDL_timer.h>

namespace engine {
    FrameController::FrameController() noexcept = default;


    void FrameController::Init() noexcept {
        if (!m_target_fps) {
            m_target_fps = m_default_fps;
        }
        m_start_time = 0;
        m_stage_start_time = 0;
        m_event_duration = {0,0};
        m_update_duration = {0,0};
        m_render_duration = {0,0};
        m_frame_duration = {0,0};
        m_other_duration = {0,0};
        m_target_delay = 1000000000/ m_target_fps;
        m_dt = 0.0f;
        m_target_frame_time = 1.0f / static_cast<double>(m_target_fps);
    }


    void FrameController::SetFPS(Uint64 fps) noexcept {
        //帧率至少为1
        m_target_fps = fps > 0 ? fps:1;
        m_target_frame_time = 1.0 / static_cast<double>(m_target_fps);
        m_target_delay = 1000000000 / m_target_fps;
    }
    float FrameController::GetFPS() const noexcept {
        return m_average_fps;
    }

    Uint64 FrameController::GetTargetFPS() const noexcept {
        return m_target_fps;
    }

    void FrameController::BeginFrame() noexcept {
        m_start_time = m_stage_start_time = SDL_GetTicksNS();

        m_event_duration[0] = m_event_duration[1];
        m_update_duration[0] = m_update_duration[1];
        m_render_duration[0] = m_render_duration[1];
        m_other_duration[0] = m_other_duration[1];
        m_other_duration[1] = 0;
    }

    void FrameController::EndEvent() noexcept {
        const auto now =SDL_GetTicksNS();
        m_event_duration[1] = now - m_stage_start_time;
        m_other_duration[1] += m_event_duration[1];
        m_stage_start_time = now;
    }

    void FrameController::EndUpdate() noexcept {
        const auto now = SDL_GetTicksNS();
        m_update_duration[1] = now - m_stage_start_time;
        m_other_duration[1] += m_update_duration[1];
        m_stage_start_time = now;
    }

    void FrameController::EndRender() noexcept {
        m_render_duration[1] = SDL_GetTicksNS() - m_stage_start_time;
        m_other_duration[1] += m_render_duration[1];
    }

    void FrameController::EndFrame() noexcept {
        auto elapsed = SDL_GetTicksNS() - m_start_time;
        if (elapsed < m_target_delay) {
            SDL_DelayNS(m_target_delay - elapsed);
            m_dt = static_cast<float>(m_target_delay) /1.0e9f;
            m_frame_duration[1] = m_target_delay;

        } else {
            m_dt = static_cast<float>(elapsed) / 1.0e9f;
            m_frame_duration[1] = elapsed;
        }
        m_other_duration[1]  = m_frame_duration[1] -m_other_duration[1];
        auto now = SDL_GetTicksNS();
        auto window_elapsed = now - m_window_start_time;
        if (window_elapsed >= 1000000000) {
            m_average_fps =static_cast<float>(m_frame_count) / static_cast<float>(window_elapsed);
            m_window_start_time = now;
            m_frame_count = 0;
        } else {
            ++m_frame_count;
        }
    }

}
