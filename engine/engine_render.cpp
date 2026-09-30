//
// @author 帽子屋小姐
// @date 2026/9/29
//

#include "engine_render.hpp"
#include "engine_log.hpp"
namespace engine {
    bool EngineRenderer::Init () {
        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;

        if (!SDL_CreateWindowAndRenderer(m_title.c_str(),m_width ,m_height,SDL_WINDOW_RESIZABLE ,&window ,&renderer)) {
            EngineLogger::Write(log_type::LOG_TYPE_FATAL,log_layer::LOG_LAYER_ENGINE ,
                "SDL 创建窗口和渲染器失败 : 调用函数 SDL_CreateWindowAndRenderer()时出错");
            return false;
        }
        window_ = EngineWindowPtr(window);
        renderer_ = EngineRendererPtr(renderer);

        return true;
    }

}