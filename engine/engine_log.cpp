//
// @author 帽子屋小姐
// @date 2026/9/29
//

#include "engine_log.hpp"
#include<spdlog/sinks/basic_file_sink.h>
namespace engine {
    std::shared_ptr<spdlog::logger> EngineLogger::logger_ = nullptr;
    bool EngineLogger::Init() {
        try {
            logger_ = spdlog::basic_logger_mt("engine","userdata/log/engine.log",false);
            logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%l]%v");
            logger_->info("Engine Logger Initialized");
            return true;
        } catch (const spdlog::spdlog_ex &ex) {
            logger_.reset();
            return false;
        }
    }

    void EngineLogger::Write(log_type type ,log_layer layer ,const char* msg) {
        //using engine::log_type;
        //using engine::log_layer;
        if (!logger_ || !msg)
            return;
        const char* layer_name = "unknown";
        switch(layer) {
            case log_layer::LOG_LAYER_ENGINE:
                layer_name = "engine";
                break;
            case log_layer::LOG_LAYER_LUA:
                layer_name = "lua";
                break;
            default:
                break;
        }
        switch (type) {
            case log_type::LOG_TYPE_INFO:
                logger_->info("[{}] {}",layer_name,msg);
                break;
            case log_type::LOG_TYPE_ERROR:
                logger_->error("[{}] {}",layer_name,msg);
                break;
            case log_type::LOG_TYPE_WARNING:
                logger_->warn("[{}] {}",layer_name,msg);
                break;
            case log_type::LOG_TYPE_FATAL:
                logger_->critical("[{}] {}",layer_name,msg);
                break;
            default:
                break;
        }
        logger_->flush();
    }

}
