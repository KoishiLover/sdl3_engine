//
// @author 帽子屋小姐
// @date 2026/9/29
//

#include "engine_log.hpp"
#include<spdlog/sinks/basic_file_sink.h>
#include<spdlog/spdlog.h>
#include<cstdio>
#include<filesystem>
#include<SDL3/SDL_messagebox.h>
namespace engine {
    bool EngineLogger::m_initialized = false ;

    bool EngineLogger::Init(std::function<void()>func) {
        if (m_initialized) {
            return true;
        }
        try {
            GetFatalHandler() = std::move(func);
            namespace fs = std::filesystem;
            auto log_path = fs::path("userdata/log/engine.log");
            fs::create_directories(log_path.parent_path());
            using fsk = spdlog::sinks::basic_file_sink_mt;
            auto file_sink = std::make_shared<fsk>(log_path.string(),true);
            auto logger = std::make_shared<spdlog::logger>("engine",spdlog::sinks_init_list{file_sink});
            logger->set_pattern("[%Y-%m-%d %H:%M:%S][%l] %v");
            spdlog::set_default_logger(logger);
            m_initialized = true;
            return true;
        } catch (const std::exception& ex) {
            std::fprintf(stderr, "EngineLogger init failed :%s\n",ex.what());
            return false;
        }
    }
    void EngineLogger::info(std::string_view message) {
        if (!message.empty())
            spdlog::info("{}" ,message);
    }
    void EngineLogger::err(std::string_view message) {
        if (!message.empty()) {
            spdlog::error("{}" ,message);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Error",message.data() ,NULL);
            GetFatalHandler()();
        }
    }
    void EngineLogger::warn(std::string_view message) {
        if (!message.empty())
            spdlog::warn("{}" ,message);
    }
}
