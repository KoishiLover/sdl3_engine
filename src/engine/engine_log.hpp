//
// @author 帽子屋小姐
// @date 2026/9/29
//

#ifndef SDL3_ENGINE_ENGINE_LOG_HPP
#define SDL3_ENGINE_ENGINE_LOG_HPP

#include<memory>
#include<string>
#include<string_view>
#include<format>
#include<functional>
namespace spdlog{class logger;}

namespace engine {
    static std::function<void()>& GetFatalHandler() {
        static std::function<void()> handler;
        return handler;
    }
    class EngineLogger {
    public:
        static bool Init(std::function<void()>func);
        static void info(std::string_view message);
        static void err(std::string_view message);
        static void warn(std::string_view message);


        template<typename... Args>
        static void info(const std::string_view fmt , Args&&... args) {
            info(std::vformat(fmt,std::make_format_args(args...)));
        }

        template<typename... Args>
        static void err(const std::string_view fmt , Args&&... args) {
            err(std::vformat(fmt,std::make_format_args(args...)));
        }

        template<typename... Args>
        static void warn(const std::string_view fmt , Args&&... args) {
            warn(std::vformat(fmt,std::make_format_args(args...)));
        }

        template<size_t Size>
        static void info(const char(&msg)[Size]) {
            info(trim(msg));
        }

        template<size_t Size>
        static void err(const char(&msg)[Size]) {
            err(trim(msg));
        }

        template<size_t Size>
        static void warn(const char(&msg)[Size]) {
            warn(trim(msg));
        }

    private :
        static bool m_initialized ;
        ///@brief 内部辅助函数,用找出将字符数组中第一个 '\0' 切割并且返回一个string_view
        static std::string_view trim(std::string_view msg) {
            if (msg.empty()) {
                return {};
            }
            for (size_t i = 0; i< msg.size(); ++i) {
                if (msg[i] == std::string_view::value_type{}) {
                    return msg.substr(0 ,i);
                }
            }
            return msg;
        }
    public:
        static EngineLogger& GetInstance() {
            static EngineLogger logger;
            return logger;
        }
    };


}


#endif //SDL3_ENGINE_ENGINE_LOG_HPP
