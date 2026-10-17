//
// @author 帽子屋小姐
// @date 2026/9/30
//

#ifndef SDL3_ENGINE_FRAME_CONTROLLER_HPP
#define SDL3_ENGINE_FRAME_CONTROLLER_HPP
#include<SDL3/SDL_time.h>
#include<array>

namespace engine {

    class FrameController {
    public:
        FrameController() noexcept;
        FrameController(const FrameController&) = delete;
        FrameController& operator=(const FrameController&) = delete;
        void Init() noexcept;
    //接口
    public:
        /**@brief 设置目标帧率，帧率最小为1*/
        void SetFPS(Uint64 fps) noexcept;

        ///@brief 获取目标帧率
        [[nodiscard]]Uint64 GetTargetFPS()const noexcept;

        ///@brief 获取真实帧率
        [[nodiscard]]float  GetFPS()const noexcept;

        ///@brief获取 dt
        [[nodiscard]]float GetDeltaTime()const noexcept;

        /**
        @brief 开始记录帧数据
        @note 必须在主循环中调用，并且主循环所有处理必须在 BeginFrame 和 EndFrame 之间处理*/
        void BeginFrame() noexcept;

        /**
        @brief 记录事件处理耗费的时间
        @note 只能在结束事件处理后调用 */
        void EndEvent() noexcept;

        /**
        @brief 记录逻辑更新所需的时间
        @note 只能在结束逻辑处理后调用*/
        void EndUpdate() noexcept;

        ///@brief 记录渲染所需的时间
        ///@note 只能在渲染结束后调用
        void EndRender()noexcept;

        ///@brief 结束一次循环,执行帧率控制
        ///@note 每次循环结束必须调用一次
        void EndFrame()noexcept;

    private:
        Uint64 m_target_fps;
        Uint64 m_start_time;
        Uint64 m_stage_start_time = 0;
        //统计数据
        float m_average_fps = 0.0;
        Uint64 m_frame_count = 0;
        Uint64 m_window_start_time = 0;

        std::array<Uint64,2> m_event_duration;
        std::array<Uint64,2> m_update_duration;
        std::array<Uint64,2> m_render_duration;
        std::array<Uint64,2> m_frame_duration;
        std::array<Uint64,2> m_other_duration;
        Uint64 m_target_delay;
        double m_dt;
        double m_target_frame_time;

        ///@details 默认参数
        static constexpr Uint64 m_default_fps = 60;
    };
}

#endif //SDL3_ENGINE_FRAME_CONTROLLER_HPP
