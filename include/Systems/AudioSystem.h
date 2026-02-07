#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

// Provide stb_vorbis declarations before pulling in miniaudio so Vorbis streams decode correctly.
#ifndef AUDIO_SYSTEM_STB_VORBIS_DECLARED
#define AUDIO_SYSTEM_STB_VORBIS_DECLARED
#ifndef STB_VORBIS_HEADER_ONLY
#define STB_VORBIS_HEADER_ONLY
#endif
#include "stb_vorbis.c"
#endif

// Enable required decoders before including miniaudio.
#ifndef MA_ENABLE_MP3
#define MA_ENABLE_MP3
#endif

#ifndef MA_ENABLE_VORBIS
#define MA_ENABLE_VORBIS
#endif

// Forward declare miniaudio structs to avoid including the massive header here if possible,
// but miniaudio isn't easily forward declarable without the header.
// For simplicity in this project, we'll include it in the CPP or just include it here if needed.
// Better practice: Pimpl idiom or just include miniaudio.h but NOT define implementation.
#include "miniaudio.h"

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    bool init();
    void shutdown();

    void setMasterVolume(float volume);
    void setMusicVolume(float volume);
    void setSfxVolume(float volume);

    void playSound(const std::string& name);
    void play3DSound(const std::string& name, const glm::vec3& position);
    void loadSound(const std::string& name, const std::string& filepath, bool isMusic = false);
    void loadMusic(const std::string& name, const std::string& filepath, bool loop = true, float gain = 1.0f);
    bool playMusic(const std::string& name, bool restartIfSame = true);
    void stopMusic();
    bool isMusicPlaying(const std::string& name) const;

    // Update listener position and orientation for 3D spatialization
    void updateListener(const glm::vec3& position, const glm::vec3& front, const glm::vec3& up);

private:
    ma_engine m_engine;
    ma_sound_group m_musicGroup;
    ma_sound_group m_sfxGroup;
    bool m_initialized = false;
    
    struct SoundData {
        std::string filepath;
        std::vector<std::unique_ptr<ma_sound>> instances;
        size_t nextInstance = 0;
        static constexpr size_t POOL_SIZE = 24; // Allow 24 overlapping instances per sound for better polyphony
    };

    struct MusicTrack {
        std::string filepath;
        std::unique_ptr<ma_sound> instance;
        bool loop = true;
        float gain = 1.0f;
    };

    std::unordered_map<std::string, SoundData> m_sounds;
    std::unordered_map<std::string, MusicTrack> m_musicTracks;
    std::string m_currentMusic;
};
