//
// @author 帽子屋小姐
// @date 2026/10/7
//

#include "Audio.hpp"
#include "../engine_log.hpp"

namespace engine {
    using log = EngineLogger;
    bool Audio::Init()noexcept {
        log::info("[engine] 开始初始化音频系统..");
        if (! MIX_Init()) {
            log::err("[engine] 初始化音频系统时出错 : {}",SDL_GetError());
            return false;
        }
        m_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK , NULL);
        if (!m_mixer) {
            log::err("[engine]  调用函数 MIX_CreateMixerDevice 时失败 : {}",SDL_GetError());
            return false;
        }
        m_bgm_track = MIX_CreateTrack(m_mixer);
        if (!m_bgm_track) {
            log::err("[engine]  创建音频轨道时失败 : {}", SDL_GetError());
            return false;
        }
        MIX_TagTrack(m_bgm_track ,"bgm");
        for (size_t i = 0;i<m_max_se_tracks;++i) {
            auto track = MIX_CreateTrack(m_mixer);
            if (!track) {
                log::err("[engine]  创建音频轨道时失败 : {}", SDL_GetError());
                return false;
            }
            MIX_TagTrack(track ,"se");
            m_se_tracks[i] = track;
        }
        log::info("[engine]  音频系统初始化完毕");
        return true;
    }

    void Audio::Destroy() {
        log::info("[engine]  开始销毁音频系统资源...");
        for (auto & v : m_audios) {
            MIX_DestroyAudio(v.second);
            v.second = nullptr;
        }
        m_audios.clear();
        for (size_t i = 0; i < m_max_se_tracks; ++i) {
            MIX_DestroyTrack(m_se_tracks[i]);
            m_se_tracks[i] = nullptr;
        }
        MIX_DestroyTrack(m_bgm_track);
        m_bgm_track = nullptr;
        MIX_DestroyMixer(m_mixer);
        m_mixer = nullptr;
        log::info("[engine]  音频资源销毁完毕");
    }

    void Audio::LoadMusic(const std::string& name,std::string_view path ,Uint32 start,Uint32 end,int loop) noexcept{
        if (m_audios.contains(name)) {
            log::warn("[lua]  BGM “{}” 已经加载，加载已取消",name);
            return;
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

        auto it = m_audios.find(name) ;
        if (it == m_audios.end()) {
            log::err("[lua]  调用函数 PlayMusic 时出错, BGM资源“{}”未找到",name);
            return;
        }
        auto [start,end,loop] = m_bgm_info.find(name)->second;
        if (!m_bgm_track) {
            log::err("[engine] 调用函数 PlayMusic 时出错，轨道为nullptr");
            return;
        }
        if (!MIX_SetTrackAudio(m_bgm_track,it->second )) {
            log::err("[engine] 调用函数 PlayMusic 时出错 : {}",SDL_GetError());
            return;
        }
        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetNumberProperty(props ,MIX_PROP_PLAY_LOOPS_NUMBER ,loop);
        SDL_SetNumberProperty(props ,MIX_PROP_PLAY_LOOP_START_MILLISECOND_NUMBER ,start*1000);
        SDL_SetNumberProperty(props ,MIX_PROP_PLAY_MAX_MILLISECONDS_NUMBER ,end*1000);
        if (!MIX_PlayTrack(m_bgm_track ,props)) {
            log::err("[engine]  播放BGM “{}”时失败",SDL_GetError());
            return;
        }
        SDL_DestroyProperties(props);
    }
    void Audio::StopMusic(std::string_view name)const noexcept {
        auto it =m_audios.find(name);
        if (it == m_audios.end()) {
            log::err("[lua]  调用函数 PlayMusic 时出错, BGM资源“{}”未找到",name);
            return;
        }
        if (it->second != MIX_GetTrackAudio(m_bgm_track)) {
            log::warn("[lua] BGM “{}” 不在播放");
            return;
        }
        MIX_StopTrack(m_bgm_track,0);
    }
    void Audio::PauseMusic(std::string_view name)const noexcept {
        auto it = m_audios.find(name);
        if (it == m_audios.end()) {
            log::err("[lua]  调用函数 PauseMusic 时出错，BGM资源“{}”未找到",name);
            return;
        }
        if (it->second != MIX_GetTrackAudio(m_bgm_track)) {
            log::err("[lua]  调用函数 PauseMusic 时出现问题 ,BGM “{}” 不在播放");
            return;
        }
        MIX_PauseTrack(m_bgm_track);
    }
    void Audio::ResumeMusic(std::string_view name)const noexcept {
        auto it = m_audios.find(name);
        if (it == m_audios.end()) {
            log::err("[lua]  调用函数 PauseMusic 时出错，BGM资源“{}”未找到",name);
            return;
        }
        if (it->second != MIX_GetTrackAudio(m_bgm_track)) {
            log::err("[lua]  调用函数 PauseMusic 时出现问题 ,BGM “{}” 不在播放");
            return;
        }
        MIX_ResumeTrack(m_bgm_track);
    }
    void Audio::SetMusicVolume(float vol)const noexcept{
        vol = abs(vol);
        vol = vol >1.0f ? 1.0f :vol;
        MIX_SetTagGain(m_mixer,"bgm",vol);
    }
    void Audio::LoadSE(const std::string& name ,std::string_view path)noexcept {
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
    }
    void Audio::PlaySE(std::string_view name) noexcept {
        auto se = m_audios.find(name)->second;
        if (!se) {
            log::err("[lua]  调用函数 PlaySE 时出错, 音效资源“{}”未找到");
            return;
        }
        int tid = -1;
        if (m_free_queue.empty()) {
            tid = m_busy_queue.front();
            m_busy_queue.pop();
        } else {
            tid = m_free_queue.front();
            m_free_queue.pop();
        }
        auto track = m_se_tracks[tid];
        m_busy_queue.push(tid);
        if (!MIX_SetTrackAudio(track ,se)) {
            log::err("[engine]  调用函数 PlaySE 时出错 : {}" ,SDL_GetError());
            return;
        }
        if (!MIX_PlayTrack(track ,NULL)){
            log::err("[engine]  调用函数 PlaySE 时出错 : {}" ,SDL_GetError());
        }
    }
    void Audio::SetSEVolume(float vol)const noexcept {
        vol = abs(vol);
        vol = vol >1.0f ? 1.0f:vol;
        if (!MIX_SetTagGain(m_mixer,"se",vol)) {
            log::err("[engine]  调用函数 SetSEVolume 时出错 : {}" ,SDL_GetError());
        }
    }
} // engine