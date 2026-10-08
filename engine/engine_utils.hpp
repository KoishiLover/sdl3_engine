//
// @author 帽子屋小姐
// @date 2026/10/3
//

#ifndef SDL3_ENGINE_ENGINE_UTILS_HPP
#define SDL3_ENGINE_ENGINE_UTILS_HPP
#include<algorithm>
#include<string>
#include<string_view>
namespace engine {
    // 让 unordered_map 可以支持string_view 的异构哈希
    struct StringHash {
        using is_transparent = void;
        size_t operator()(std::string_view sv) const{
            return std::hash<std::string_view>{}(sv);
        }
    };
    struct StringEqual {
        using is_transparent = void;
        bool operator()(std::string_view sv1, std::string_view sv2) const{
            return sv1 == sv2;
        }
    };
}

#endif //SDL3_ENGINE_ENGINE_UTILS_HPP
