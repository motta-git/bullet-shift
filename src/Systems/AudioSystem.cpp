#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#define MINIAUDIO_IMPLEMENTATION
#include "AudioSystem.h"
#pragma GCC diagnostic pop
#include "Settings.h"
#include <iostream>

#if defined(STB_VORBIS_HEADER_ONLY)
#undef STB_VORBIS_HEADER_ONLY
#endif
#include "stb_vorbis.c"

AudioSystem::AudioSystem() {
}

AudioSystem::~AudioSystem() {
    shutdown();
}

bool AudioSystem::init() {
    ma_result result;
    
    ma_engine_config engineConfig = ma_engine_config_init();
    // Reduce latency by tuning the period size. 10ms is usually a good balance.
    engineConfig.periodSizeInMilliseconds = 10;
    
    result = ma_engine_init(&engineConfig, &m_engine);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize audio engine." << std::endl;
        return false;
    }
    
    // Initialize sound groups
    result = ma_sound_group_init(&m_engine, 0, NULL, &m_musicGroup);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize music group." << std::endl;
        return false;
    }

    result = ma_sound_group_init(&m_engine, 0, NULL, &m_sfxGroup);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize SFX group." << std::endl;
        return false;
    }

    // Apply initial volumes from Settings
    const auto& settings = Settings::getInstance();
    setMasterVolume(settings.audio.masterVolume);
    setMusicVolume(settings.audio.musicVolume); 
    setSfxVolume(settings.audio.sfxVolume);

    m_initialized = true;
    return true;
}

void AudioSystem::setMasterVolume(float volume) {
    if (!m_initialized) return;
    ma_engine_set_volume(&m_engine, volume);
}

void AudioSystem::setMusicVolume(float volume) {
    if (!m_initialized) return;
    ma_sound_group_set_volume(&m_musicGroup, volume);
}

void AudioSystem::setSfxVolume(float volume) {
    if (!m_initialized) return;
    ma_sound_group_set_volume(&m_sfxGroup, volume);
}

void AudioSystem::shutdown() {
    if (m_initialized) {
        for (auto& track : m_musicTracks) {
            if (track.second.instance) {
                ma_sound_stop(track.second.instance.get());
                ma_sound_uninit(track.second.instance.get());
            }
        }
        m_musicTracks.clear();
        m_currentMusic.clear();

        for (auto& pair : m_sounds) {
            for (auto& sound : pair.second.instances) {
                ma_sound_uninit(sound.get());
            }
        }
        m_sounds.clear();

        ma_sound_group_uninit(&m_musicGroup);
        ma_sound_group_uninit(&m_sfxGroup);
        ma_engine_uninit(&m_engine);
        m_initialized = false;
    }
}

void AudioSystem::loadSound(const std::string& name, const std::string& filepath, bool isMusic) {
    if (!m_initialized) return;

    // Check if sound already exists to avoid redundant loading and potential leaks
    if (m_sounds.find(name) != m_sounds.end()) {
        return;
    }

    SoundData data;
    data.filepath = filepath;
    
    ma_sound_group* pGroup = isMusic ? &m_musicGroup : &m_sfxGroup;

    for (size_t i = 0; i < SoundData::POOL_SIZE; ++i) {
        auto sound = std::make_unique<ma_sound>();
        // Use MA_SOUND_FLAG_DECODE to load the whole sound into memory for better performance with overlapping instances
        ma_result result = ma_sound_init_from_file(&m_engine, filepath.c_str(), MA_SOUND_FLAG_DECODE, pGroup, NULL, sound.get());
        if (result != MA_SUCCESS) {
            // Only log once per file to avoid spamming
            if (i == 0) {
                std::cerr << "Failed to load sound file: " << filepath << " (Error: " << result << ")" << std::endl;
            }
            continue;
        }
        data.instances.push_back(std::move(sound));
    }

    if (!data.instances.empty()) {
        m_sounds[name] = std::move(data);
    }
}

void AudioSystem::loadMusic(const std::string& name, const std::string& filepath, bool loop, float gain) {
    if (!m_initialized || filepath.empty()) return;

    if (m_musicTracks.find(name) != m_musicTracks.end()) {
        return;
    }

    MusicTrack track;
    track.filepath = filepath;
    track.loop = loop;
    track.gain = gain;

    ma_result result = MA_SUCCESS;
    auto createSound = [&](ma_uint32 flags) -> std::unique_ptr<ma_sound> {
        auto instance = std::make_unique<ma_sound>();
        result = ma_sound_init_from_file(&m_engine, filepath.c_str(), flags, &m_musicGroup, NULL, instance.get());
        if (result != MA_SUCCESS) {
            return nullptr;
        }
        return instance;
    };

    auto sound = createSound(MA_SOUND_FLAG_STREAM);
    if (!sound) {
        std::cerr << "Streaming load failed for music file: " << filepath
                  << " (Error: " << result << ") - retrying with full decode." << std::endl;
        sound = createSound(MA_SOUND_FLAG_DECODE);
        if (!sound) {
            std::cerr << "Failed to load music file: " << filepath << " (Error: " << result << ")" << std::endl;
            return;
        }
    }

    ma_sound_set_looping(sound.get(), loop ? MA_TRUE : MA_FALSE);
    ma_sound_set_volume(sound.get(), gain);
    ma_sound_set_spatialization_enabled(sound.get(), MA_FALSE);
    ma_sound_set_attenuation_model(sound.get(), ma_attenuation_model_none);

    track.instance = std::move(sound);
    m_musicTracks[name] = std::move(track);
}

bool AudioSystem::playMusic(const std::string& name, bool restartIfSame) {
    if (!m_initialized) return false;

    auto it = m_musicTracks.find(name);
    if (it == m_musicTracks.end()) {
        std::cerr << "Music track not loaded: " << name << std::endl;
        return false;
    }

    if (!m_currentMusic.empty() && m_currentMusic != name) {
        auto current = m_musicTracks.find(m_currentMusic);
        if (current != m_musicTracks.end() && current->second.instance) {
            std::cout << "[AudioSystem] Stopping track '" << m_currentMusic
                      << "' before playing '" << name << "'" << std::endl;
            ma_sound_stop(current->second.instance.get());
        }
    }

    ma_sound* sound = it->second.instance.get();
    if (!sound) return false;

    if (m_currentMusic == name) {
        if (!restartIfSame && ma_sound_is_playing(sound)) {
            std::cout << "[AudioSystem] Track '" << name << "' already playing; skipping restart." << std::endl;
            return true;
        }
        ma_result stopResult = ma_sound_stop(sound);
        if (stopResult != MA_SUCCESS) {
            std::cerr << "Failed to stop existing music track: " << name << " (Error: " << stopResult << ")" << std::endl;
        }
    }

    ma_result seekResult = ma_sound_seek_to_pcm_frame(sound, 0);
    if (seekResult != MA_SUCCESS) {
        std::cerr << "Failed to seek music track: " << name << " (Error: " << seekResult << ")" << std::endl;
    }

    ma_result startResult = ma_sound_start(sound);
    if (startResult != MA_SUCCESS) {
        std::cerr << "Failed to start music track: " << name << " (Error: " << startResult << ")" << std::endl;
        return false;
    }
    std::cout << "[AudioSystem] Started track '" << name << "' (loop="
              << (it->second.loop ? "on" : "off") << ", gain=" << it->second.gain << ")" << std::endl;
    m_currentMusic = name;
    return true;
}

void AudioSystem::stopMusic() {
    if (!m_initialized || m_currentMusic.empty()) return;

    auto it = m_musicTracks.find(m_currentMusic);
    if (it != m_musicTracks.end() && it->second.instance) {
        ma_sound_stop(it->second.instance.get());
    }

    m_currentMusic.clear();
}

bool AudioSystem::isMusicPlaying(const std::string& name) const {
    if (!m_initialized) {
        return false;
    }

    auto it = m_musicTracks.find(name);
    if (it == m_musicTracks.end() || !it->second.instance) {
        return false;
    }

    return ma_sound_is_playing(it->second.instance.get()) == MA_TRUE;
}

void AudioSystem::playSound(const std::string& name) {
    if (!m_initialized) return;

    auto it = m_sounds.find(name);
    if (it != m_sounds.end() && !it->second.instances.empty()) {
        auto& data = it->second;
        ma_sound* sound = data.instances[data.nextInstance].get();
        
        ma_sound_set_spatialization_enabled(sound, MA_FALSE);
        ma_sound_seek_to_pcm_frame(sound, 0);
        ma_sound_start(sound);
        
        data.nextInstance = (data.nextInstance + 1) % data.instances.size();
    }
}

void AudioSystem::play3DSound(const std::string& name, const glm::vec3& position) {
    if (!m_initialized) return;

    auto it = m_sounds.find(name);
    if (it != m_sounds.end() && !it->second.instances.empty()) {
        auto& data = it->second;
        ma_sound* sound = data.instances[data.nextInstance].get();
        
        ma_sound_set_spatialization_enabled(sound, MA_TRUE);
        ma_sound_set_position(sound, position.x, position.y, position.z);
        
        // Tuning for better audibility at distance
        // Increased to 25.0f so sounds stay loud until 25 meters away, then drop off.
        ma_sound_set_min_distance(sound, 25.0f);
        ma_sound_set_attenuation_model(sound, ma_attenuation_model_inverse);
        ma_sound_seek_to_pcm_frame(sound, 0);
        ma_sound_start(sound);
        
        data.nextInstance = (data.nextInstance + 1) % data.instances.size();
    }
}

void AudioSystem::updateListener(const glm::vec3& position, const glm::vec3& front, const glm::vec3& up) {
    if (!m_initialized) return;

    ma_engine_listener_set_position(&m_engine, 0, position.x, position.y, position.z);
    ma_engine_listener_set_direction(&m_engine, 0, front.x, front.y, front.z);
    ma_engine_listener_set_world_up(&m_engine, 0, up.x, up.y, up.z);
}
