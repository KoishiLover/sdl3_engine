//
// @author 帽子屋小姐
// @date 2026/10/7
//

#ifndef HATTA_ENGINE_AUDIO_HPP
#define HATTA_ENGINE_AUDIO_HPP

#include <SDL3_mixer/SDL_mixer.h>
#include <array>
#include<string>
#include <unordered_map>
#include<queue>
#include "../engine_utils.hpp"
namespace engine {
    struct MusicLoopInfo {
        Uint32 start = 0;
        Uint32 end  = 0;
        int loop = -1;
    };
    class Audio {
    public:
        Audio() = default;
        ~Audio() =default;
        bool Init()noexcept;
        void Destroy();
    private:
        MIX_Mixer* m_mixer;
        std::unordered_map<std::string ,MIX_Audio*,StringHash,StringEqual> m_audios;
        std::unordered_map<std::string,MusicLoopInfo,StringHash,StringEqual> m_bgm_info;
        std::array<MIX_Track* ,32> m_se_tracks;
        MIX_Track* m_bgm_track;
        Uint8 m_max_se_tracks = 32;
        std::queue<int> m_free_queue;
        std::queue<int> m_busy_queue;
    public:
        void LoadMusic(const std::string& name,std::string_view path ,Uint32 start,Uint32 end ,int loop) noexcept;
        void PlayMusic(std::string_view name) const noexcept;
        void StopMusic(std::string_view name)const noexcept;
        void PauseMusic(std::string_view name)const noexcept;
        void ResumeMusic(std::string_view name)const noexcept;
        void SetMusicVolume(float vol)const noexcept;
        void LoadSE(const std::string& name ,std::string_view path)noexcept;
        void PlaySE(std::string_view name)  noexcept;
        void SetSEVolume(float vol)const noexcept;
    };
} // engine

#endif //HATTA_ENGINE_AUDIO_HPP
