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
#include <vector>
#include <cstddef>
#include<queue>
#include "../engine_utils.hpp"
namespace engine {
    ///@brief 一条BGM的循环信息
    ///@note start / end 的单位为秒，0 表示从文件开头 / 播放到文件结尾
    struct MusicLoopInfo {
        Uint32 start = 0;
        Uint32 end  = 0;
        int loop = -1;
    };
    class Audio {
    public:
        Audio() = default;
        ~Audio() =default;
        ///@brief 初始化音频系统。可重复调用，已初始化时直接返回 true
        bool Init()noexcept;
        ///@brief 销毁音频系统资源。可重复调用（幂等）
        void Destroy()noexcept;
    private:
        ///@brief SE 轨道池容量（BGM 轨道之外的轨道总数）
        static constexpr size_t m_max_se_tracks = 32;

        MIX_Mixer* m_mixer = nullptr;
        std::unordered_map<std::string ,MIX_Audio*,StringHash,StringEqual> m_audios;
        std::unordered_map<std::string,MusicLoopInfo,StringHash,StringEqual> m_bgm_info;
        std::vector<MIX_Track*> m_se_tracks;
        MIX_Track* m_bgm_track = nullptr;

        ///@brief 空闲轨道下标，队首为最早归还的轨道
        std::queue<size_t> m_free_queue;
        ///@brief 正在播放的轨道下标，队首为最早开始播放的轨道（轨道用尽时优先抢占它）
        std::queue<size_t> m_busy_queue;

        ///@brief 当前装载在BGM轨道上的资源，nullptr 表示BGM轨道空闲
        ///@note 声明为 mutable 是因为播放控制接口是 const（引擎侧以 const 对象调用），
        ///      但「当前播放的是哪条BGM」属于内部记账，不影响调用者可见的逻辑状态
        mutable MIX_Audio* m_current_bgm = nullptr;

        ///@brief 创建所有轨道并把SE轨道分配器的初始状态准备好
        bool CreateTracks()noexcept;
        ///@brief 按名字取音频资源，找不到时记录警告并返回 nullptr
        ///@param caller 调用方接口名，仅用于日志
        [[nodiscard]] MIX_Audio* FindAudio(std::string_view caller , std::string_view name)const noexcept;
        ///@brief 取得一条可用的SE轨道下标；全部占用时抢占最早播放的那条
        [[nodiscard]] size_t AcquireSETrack()noexcept;
        ///@brief 把轨道归还给空闲队列
        void ReleaseSETrack(size_t id)noexcept;
    public:
        ///@param start BGM循环回跳点（秒）
        ///@param end   播放终点（秒），0 表示播放到文件结束
        ///@param loop  循环次数，-1 表示无限循环
        void LoadMusic(const std::string& name,std::string_view path ,Uint32 start,Uint32 end ,int loop) noexcept;
        //注意：以下播放控制接口保持 const（引擎侧以 const 对象调用它们），
        //      内部记账字段声明为 mutable
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
