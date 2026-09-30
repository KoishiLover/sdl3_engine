//
// @author 帽子屋小姐
// @date 2026/9/29
//

#ifndef SDL3_ENGINE_ENGINE_LOG_HPP
#define SDL3_ENGINE_ENGINE_LOG_HPP
#include<spdlog/spdlog.h>

namespace engine {
    enum class log_type {
        LOG_TYPE_INFO ,
        LOG_TYPE_WARNING ,
        LOG_TYPE_ERROR ,
        LOG_TYPE_FATAL
    };
    enum class log_layer {
        LOG_LAYER_ENGINE,
        LOG_LAYER_LUA
    };

    class EngineLogger {
    public:
        static bool Init();
        static void Write(log_type type,log_layer layer ,const char* message);
        std::string log_path = "userdata/log/engine.log";
    private:
        static std::shared_ptr<spdlog::logger> logger_ ;
    };
}


#endif //SDL3_ENGINE_ENGINE_LOG_HPP
