//
// @author 帽子屋小姐
// @date 2026/10/7
//

#include "Audio.hpp"
#include "../engine_log.hpp"
#include<algorithm>
#include<cmath>

namespace engine {
    using log = EngineLogger;
    namespace {
        ///@brief 把音量钳制到 Mixer 允许的 [0.0 , 1.0] 区间
        ///@note SDL_mixer 中负增益是非法的，因此负数（含 NaN）一律视为 0
        [[nodiscard]] float ClampVolume(float vol) noexcept {
            if (std::isnan(vol) || vol <= 0.0f) {
                return 0.0f;
            }
            return vol > 1.0f ? 1.0f : vol;
        }
    }

    bool Audio::CreateTracks()noexcept {
        m_bgm_track = MIX_CreateTrack(m_mixer);
        if (!m_bgm_track) {
            log::err("[engine]  创建BGM轨道时失败 : {}", SDL_GetError());
            return false;
        }
        MIX_TagTrack(m_bgm_track ,"bgm");

        m_se_tracks.resize(m_max_se_tracks ,nullptr);
        for (size_t i = 0; i < m_max_se_tracks; ++i) {
            auto track = MIX_CreateTrack(m_mixer);
            if (!track) {
                log::err("[engine]  创建SE轨道时失败 : {}", SDL_GetError());
                return false;
            }
            MIX_TagTrack(track ,"se");
            m_se_tracks[i] = track;
            //所有SE轨道初始都是空闲的
            m_free_queue.push(i);
        }
        return true;
    }

    bool Audio::Init()noexcept {
        //已经初始化过，直接返回，避免重复创建混音器
        if (m_mixer) {
            return true;
        }
        log::info("[engine] 开始初始化音频系统..");
        if (! MIX_Init()) {
            log::err("[engine] 初始化音频系统时出错 : {}",SDL_GetError());
            return false;
        }
        m_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK , NULL);
        if (!m_mixer) {
            log::err("[engine]  调用函数 MIX_CreateMixerDevice 时失败 : {}",SDL_GetError());
            MIX_Quit();
            return false;
        }
        if (!CreateTracks()) {
            //创建轨道失败，回滚已建立的混音器，避免留下「m_mixer 非空却不可用」的中间态
            Destroy();
            return false;
        }
        log::info("[engine]  音频系统初始化完毕");
        return true;
    }

    void Audio::Destroy()noexcept {
        log::info("[engine]  开始销毁音频系统资源...");

        for (auto & v : m_audios) {
            if (v.second) {
                MIX_DestroyAudio(v.second);
                v.second = nullptr;
            }
        }
        m_audios.clear();
        m_bgm_info.clear();

        for (auto*& track : m_se_tracks) {
            if (track) {
                MIX_DestroyTrack(track);
                track = nullptr;
            }
        }
        m_se_tracks.clear();

        if (m_bgm_track) {
            MIX_DestroyTrack(m_bgm_track);
            m_bgm_track = nullptr;
        }

        if (m_mixer) {
            MIX_DestroyMixer(m_mixer);
            m_mixer = nullptr;
        }

        //清空轨道分配器状态
        std::queue<size_t> empty_free;
        std::queue<size_t> empty_busy;
        m_free_queue.swap(empty_free);
        m_busy_queue.swap(empty_busy);

        m_current_bgm = nullptr;

        //MIX_Init 与 MIX_Quit 是成对的全局调用，只有真的初始化过才退出
        MIX_Quit();
        log::info("[engine]  音频资源销毁完毕");
    }

    MIX_Audio* Audio::FindAudio(std::string_view caller , std::string_view name)const noexcept {
        auto it = m_audios.find(name);
        if (it == m_audios.end()) {
            //脚本里写错资源名不应该终止整个引擎，因此只记录警告并忽略本次调用
            log::warn("[lua]  调用函数 {} 时忽略 : 音频资源“{}”未找到，请先调用 LoadMusic / LoadSE",caller,name);
            return nullptr;
        }
        return it->second;
    }

    void Audio::LoadMusic(const std::string& name,std::string_view path ,Uint32 start,Uint32 end,int loop) noexcept{
        if (!m_mixer) {
            log::err("[lua]  调用函数 LoadMusic 时出错 : 音频系统尚未初始化");
            return;
        }
        if (m_audios.contains(name)) {
            log::warn("[lua]  BGM “{}” 已经加载，加载已取消",name);
            return;
        }
        if (end != 0 && end <= start) {
            log::warn("[lua]  BGM “{}” 的播放终点({}秒)不大于循环起点({}秒)，已按播放到文件结尾处理",
                      name,end,start);
            end = 0;
        }
        auto audio = MIX_LoadAudio(m_mixer,path.data(),true);
        if (!audio) {
            log::err("[lua]  从路径“{}” 加载BGM资源 “{}”时出错 : {}",path,name ,SDL_GetError());
            return;
        }
        m_audios[name] = audio;
        m_bgm_info[name] = {.start = start ,.end = end ,.loop = loop};
        log::info("[lua]  已从路径“{}”加载 BGM资源 “{}”",path,name);
    }

    void Audio::PlayMusic(std::string_view name)const noexcept {
        if (!m_bgm_track) {
            log::err("[engine] 调用函数 PlayMusic 时出错，轨道为nullptr");
            return;
        }
        auto audio = FindAudio("PlayMusic" ,name);
        if (!audio) {
            return;
        }
        auto info = m_bgm_info.find(name);
        if (info == m_bgm_info.end()) {
            log::warn("[lua]  调用函数 PlayMusic 时忽略 : BGM资源“{}”不是音乐资源（缺少循环信息）",name);
            return;
        }
        if (!MIX_SetTrackAudio(m_bgm_track,audio)) {
            log::err("[engine] 调用函数 PlayMusic 时出错 : {}",SDL_GetError());
            return;
        }
        m_current_bgm = audio;

        auto [start,end,loop] = info->second;
        SDL_PropertiesID props = SDL_CreateProperties();
        if (!props) {
            log::err("[engine]  播放BGM “{}”时无法创建属性集 : {}",name,SDL_GetError());
            return;
        }
        //start / end 的单位为秒，SDL_mixer 需要毫秒
        SDL_SetNumberProperty(props ,MIX_PROP_PLAY_LOOPS_NUMBER ,loop);
        if (start > 0) {
            SDL_SetNumberProperty(props ,MIX_PROP_PLAY_LOOP_START_MILLISECOND_NUMBER ,
                                  static_cast<Sint64>(start) * 1000);
        }
        if (end > 0) {
            SDL_SetNumberProperty(props ,MIX_PROP_PLAY_MAX_MILLISECONDS_NUMBER ,
                                  static_cast<Sint64>(end) * 1000);
        }
        const bool ok = MIX_PlayTrack(m_bgm_track ,props);
        //无论成功失败都要释放属性集
        SDL_DestroyProperties(props);
        if (!ok) {
            log::err("[engine]  播放BGM “{}”时失败 : {}",name,SDL_GetError());
        }
    }

    void Audio::StopMusic(std::string_view name)const noexcept {
        auto audio = FindAudio("StopMusic" ,name);
        if (!audio) {
            return;
        }
        if (!m_bgm_track || audio != m_current_bgm) {
            log::warn("[lua] BGM “{}” 不在播放",name);
            return;
        }
        if (!MIX_TrackPlaying(m_bgm_track) && !MIX_TrackPaused(m_bgm_track)) {
            log::warn("[lua] BGM “{}” 不在播放",name);
            return;
        }
        if (!MIX_StopTrack(m_bgm_track,0)) {
            log::err("[engine]  停止BGM “{}”时出错 : {}",name,SDL_GetError());
        }
    }

    void Audio::PauseMusic(std::string_view name)const noexcept {
        auto audio = FindAudio("PauseMusic" ,name);
        if (!audio) {
            return;
        }
        if (!m_bgm_track || audio != m_current_bgm) {
            log::err("[lua]  调用函数 PauseMusic 时出现问题 ,BGM “{}” 不在播放",name);
            return;
        }
        if (!MIX_TrackPlaying(m_bgm_track)) {
            log::err("[lua]  调用函数 PauseMusic 时出现问题 ,BGM “{}” 不在播放",name);
            return;
        }
        if (!MIX_PauseTrack(m_bgm_track)) {
            log::err("[engine]  暂停BGM “{}”时出错 : {}",name,SDL_GetError());
        }
    }

    void Audio::ResumeMusic(std::string_view name)const noexcept {
        auto audio = FindAudio("ResumeMusic" ,name);
        if (!audio) {
            return;
        }
        if (!m_bgm_track || audio != m_current_bgm) {
            log::err("[lua]  调用函数 ResumeMusic 时出现问题 ,BGM “{}” 不在播放",name);
            return;
        }
        if (!MIX_TrackPaused(m_bgm_track)) {
            log::err("[lua]  调用函数 ResumeMusic 时出现问题 ,BGM “{}” 未被暂停",name);
            return;
        }
        if (!MIX_ResumeTrack(m_bgm_track)) {
            log::err("[engine]  恢复BGM “{}”时出错 : {}",name,SDL_GetError());
        }
    }

    void Audio::SetMusicVolume(float vol)const noexcept {
        const float clamped = ClampVolume(vol);
        if (!m_mixer) {
            log::err("[engine]  调用函数 SetMusicVolume 时出错 : 音频系统尚未初始化");
            return;
        }
        if (!MIX_SetTagGain(m_mixer,"bgm",clamped)) {
            log::err("[engine]  调用函数 SetMusicVolume 时出错 : {}" ,SDL_GetError());
        }
    }

    void Audio::LoadSE(const std::string& name ,std::string_view path)noexcept {
        if (!m_mixer) {
            log::err("[lua]  调用函数 LoadSE 时出错 : 音频系统尚未初始化");
            return;
        }
        if (m_audios.contains(name)) {
            log::warn("[lua]  音效文件“{}”,已存在，加载已取消",name);
            return;
        }
        auto audio = MIX_LoadAudio(m_mixer ,path.data(),true);
        if (!audio) {
            log::err("[lua]  从路径“{}” 加载音效资源 “{}”时出错 : {}",path,name,SDL_GetError());
            return;
        }
        m_audios[name] = audio;
        log::info("[lua]  已从路径“{}”加载 音效资源 “{}”",path,name);
    }

    size_t Audio::AcquireSETrack()noexcept {
        if (!m_free_queue.empty()) {
            const size_t id = m_free_queue.front();
            m_free_queue.pop();
            return id;
        }
        //所有轨道都在使用中：抢占最早开始播放的那条，保证音效不会静默丢失
        const size_t id = m_busy_queue.front();
        m_busy_queue.pop();
        MIX_StopTrack(m_se_tracks[id],0);
        return id;
    }

    void Audio::ReleaseSETrack(size_t id)noexcept {
        if (id < m_se_tracks.size()) {
            m_free_queue.push(id);
        }
    }

    void Audio::PlaySE(std::string_view name) noexcept {
        if (!m_mixer || m_se_tracks.empty()) {
            log::err("[lua]  调用函数 PlaySE 时出错 : 音频系统尚未初始化");
            return;
        }
        auto audio = FindAudio("PlaySE" ,name);
        if (!audio) {
            return;
        }
        //回收本帧之前已经播放完的轨道，避免长时间运行时轨道池被占满
        const size_t busy_count = m_busy_queue.size();
        for (size_t i = 0; i < busy_count; ++i) {
            const size_t id = m_busy_queue.front();
            m_busy_queue.pop();
            if (MIX_TrackPlaying(m_se_tracks[id])) {
                m_busy_queue.push(id);
            } else {
                ReleaseSETrack(id);
            }
        }

        const size_t tid = AcquireSETrack();
        auto track = m_se_tracks[tid];
        m_busy_queue.push(tid);

        if (!MIX_SetTrackAudio(track ,audio)) {
            log::err("[engine]  调用函数 PlaySE 时出错 : {}" ,SDL_GetError());
            return;
        }
        //音效音量由 SetSEVolume 设置的 tag gain 统一管理，无需逐轨道设置
        if (!MIX_PlayTrack(track ,NULL)){
            log::err("[engine]  播放音效 “{}” 时出错 : {}" ,name,SDL_GetError());
        }
    }

    void Audio::SetSEVolume(float vol)const noexcept {
        const float clamped = ClampVolume(vol);
        if (!m_mixer) {
            log::err("[engine]  调用函数 SetSEVolume 时出错 : 音频系统尚未初始化");
            return;
        }
        //按标签统一设置，每条SE轨道（含尚未被使用过的）都会带上这个增益
        if (!MIX_SetTagGain(m_mixer,"se",clamped)) {
            log::err("[engine]  调用函数 SetSEVolume 时出错 : {}" ,SDL_GetError());
        }
    }
} // engine
