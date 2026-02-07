#pragma once

#include <array>
#include <glm/glm.hpp>
#include "Weapon.h"

namespace Config {
    // ... rest of physics etc ...
    // Physics constants
    constexpr float GRAVITY = 20.0f;
    constexpr float JUMP_FORCE = 17.0f; // Higher jump
    constexpr float MOVE_SPEED = 12.0f; // Faster max movement
    constexpr float PLAYER_ACCELERATION = 60.0f; // Fast acceleration
    constexpr float PLAYER_DECELERATION = 40.0f; // Quick stopping
    constexpr float FALL_DEATH_THRESHOLD = -20.0f; // Y position below which player dies
    
    // Particle system
    constexpr int MAX_PARTICLES = 2000;
    
    // Player dimensions (realistic human proportions)
    constexpr float PLAYER_WIDTH = 0.6f;
    constexpr float PLAYER_HEIGHT = 1.8f;
    constexpr float PLAYER_DEPTH = 0.6f;
    constexpr float EYE_HEIGHT = 0.7f; // Relative to center (0.9m)

    // Spawn offsets
    // Extra buffer added to spawn Y to avoid first-frame jitter/tunneling
    constexpr float SPAWN_BUFFER = 0.25f;
    inline constexpr float SPAWN_HALF_HEIGHT = PLAYER_HEIGHT / 2.0f;
    
    // Player Dash settings
    constexpr float DASH_DURATION = 1.0f;
    constexpr float DASH_SPEED = 30.0f;
    constexpr float DASH_COOLDOWN = 3.0f; // Increased as requested
    
    // Camera settings
    constexpr float FOV = 45.0f;
    constexpr float NEAR_PLANE = 0.1f;
    constexpr float FAR_PLANE = 100.0f;
    
    // Bullet Time settings
    constexpr float MAX_BULLET_TIME_ENERGY = 100.0f;
    constexpr float BULLET_TIME_DRAIN_RATE = 25.0f; // Drains in 4 seconds
    constexpr float BULLET_TIME_REGEN_RATE = 15.0f; // Regens in ~7 seconds
    constexpr float MIN_BULLET_TIME_SCALE = 0.2f;
    
    // Camera settings
    namespace Camera {
        constexpr float YAW = -90.0f;
        constexpr float PITCH = 0.0f;
        constexpr float SPEED = 5.0f;
        constexpr float SENSITIVITY = 0.1f;
        constexpr float ZOOM = 45.0f;
        constexpr float RECOIL_RECOVERY_SPEED = 10.0f;
    }
    
    // Rendering
    constexpr int MSAA_SAMPLES = 4;
    constexpr int DEPTH_BITS = 24;
    constexpr int STENCIL_BITS = 8;
    
    // UI Settings
    constexpr float UI_REFERENCE_HEIGHT = 720.0f; // Lower value = Bigger UI
    constexpr const char* FONT_PATH = "assets/ui/Airlock.otf";

    namespace Audio {
        struct MusicTrackConfig {
            const char* id;
            const char* filePath;
            bool loop;
            float gain;
        };

        inline constexpr MusicTrackConfig MAIN_MENU_TRACK{
            "music_main_menu",
            "assets/sounds/music/Song_2.ogg",
            true,
            1.0f
        };

        inline constexpr std::array<MusicTrackConfig, 3> LEVEL_MUSIC{{
            {"music_level_1", "assets/sounds/music/drum_and_bass.ogg", true, 1.0f},
            {"music_level_2", "assets/sounds/music/drum_and_bass_2.ogg", true, 1.0f},
            {"music_level_3", "assets/sounds/music/drum_and_bass_3.ogg", true, 1.0f}
        }};

        inline constexpr const MusicTrackConfig& getLevelMusic(int levelIndex) {
            return (levelIndex >= 1 && static_cast<size_t>(levelIndex) <= LEVEL_MUSIC.size())
                ? LEVEL_MUSIC[static_cast<size_t>(levelIndex - 1)]
                : MAIN_MENU_TRACK;
        }

        inline constexpr const char* FOOTSTEP_SOUND_1 = "assets/sounds/sfx/footstep_1.ogg";
        inline constexpr const char* FOOTSTEP_SOUND_2 = "assets/sounds/sfx/footstep_2.ogg";
        inline constexpr const char* UI_CLICK_SOUND = "assets/sounds/ui/Minimalist13.ogg";
        inline constexpr const char* UI_CANCEL_SOUND = "assets/sounds/ui/Minimalist10.ogg";
        constexpr float STEP_DISTANCE = 2.5f; // Distance in meters between step sounds
    }

    // Weapon Settings
    namespace Weapon {
        // Shared animation settings
        constexpr float IDLE_BOB_SPEED = 4.0f;
        constexpr float MOVE_BOB_SPEED = 11.0f;
        constexpr float RECOIL_RECOVERY_SPEED = 10.0f;

        struct WeaponConfig {
            const char* name;
            const char* modelPath;
            const char* fireSoundPath;
            const char* reloadSoundPath;
            float scale;
            glm::vec3 offset;
            glm::vec3 rotation;
            float recoilAmount;
            float recoilRotation;
            int maxAmmo;
            int initialReserve;
            float fireRate;
            float damage;
            float range;
            float projectileSpeed;
            float projectileLifetime;
            int projectileCount;
            float spread;
            float reloadTime;
            float pumpTime; // For pump-action weapons; 0 for none
        };

        inline WeaponConfig getWeaponConfig(WeaponType type) {
            switch (type) {
                case WeaponType::PISTOL:
                    return {
                        "Pistol",
                        "assets/models/pistol.glb",
                        "assets/sounds/sfx/pistol-fire.ogg",
                        "assets/sounds/sfx/gun-reload.ogg",
                        0.02f,
                        glm::vec3(0.18f, -0.1f, -0.65f),
                        glm::vec3(0.0f, 181.0f, -90.0f),
                        2.5f, // recoilAmount (Camera)
                        30.0f, // recoilRotation (Weapon Animation)
                        12,
                        48,
                        3.0f,
                        25.0f,
                        50.0f,
                        50.0f,
                        3.0f,
                        1, 0.0f,
                        1.5f, // Reload time
                        0.0f  // pumpTime
                    };
                case WeaponType::RIFLE:
                    return {
                        "Rifle",
                        "assets/models/ak_rifle.glb",
                        "assets/sounds/sfx/rifle-fire.ogg",
                        "assets/sounds/sfx/gun-reload.ogg",
                        0.015f,
                        glm::vec3(0.32f, -0.45f, -1.5f),
                        glm::vec3(0.0f, 90.0f, 0.0f),
                        1.8f, // recoilAmount (Camera)
                        30.0f, // recoilRotation (Weapon Animation)
                        30,
                        120,
                        10.0f,
                        15.0f,
                        100.0f,
                        60.0f,
                        5.0f,
                        1, 0.0f,
                        2.0f, // Reload time
                        0.0f  // pumpTime
                    };
                case WeaponType::AUTO_SHOTGUN:
                    return {
                        "Auto-Shotgun",
                        "assets/models/auto-shotgun.glb", // Placeholder
                        "assets/sounds/sfx/auto-shotgun-fire.ogg",
                        "assets/sounds/sfx/gun-reload.ogg",
                        0.015f, // Placeholder scale
                        glm::vec3(0.32f, -0.45f, -1.5f), // Placeholder offset
                        glm::vec3(0.0f, 90.0f, 0.0f), // Placeholder rotation
                        5.0f, // High recoil
                        45.0f, // High visual kick
                        8,
                        32,
                        5.0f, // Fire rate
                        15.0f, // Damage per pellet
                        20.0f, // Short range
                        40.0f, // Slower projectiles
                        1.5f,
                        8, // 8 pellets
                        0.05f, // Spread
                        0.6f, // Reload time per shell
                        0.0f // pumpTime (auto)
                    };
                case WeaponType::PUMP_SHOTGUN:
                    return {
                        "Pump-Shotgun",
                        "assets/models/auto-shotgun.glb", // Reuse model for now
                        "assets/sounds/sfx/pump-fire.ogg",
                        "assets/sounds/sfx/gun-reload.ogg",
                        0.015f, // scale
                        glm::vec3(0.32f, -0.45f, -1.5f),
                        glm::vec3(0.0f, 90.0f, 0.0f),
                        5.0f, // Recoil amount
                        45.0f, // visual kick
                        6, // maxAmmo
                        24, // reserve
                        2.0f, // Fire rate (slower, pump action)
                        16.0f, // Damage per pellet
                        22.0f, // range
                        45.0f, // projectile speed
                        1.2f, // projectile lifetime
                        8, // pellets
                        0.05f, // spread
                        0.6f, // reload time per shell
                        0.42f // pumpTime in seconds
                    };
                default:
                    return { "Unknown", "", "", "", 1.0f, glm::vec3(0.0f), glm::vec3(0.0f), 0.1f, 10.0f, 0, 0, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1, 0.0f, 1.0f, 0.0f };
            }
        }
    }

    // Level Configurations
    namespace Levels {
        struct LevelConfig {
            const char* name;
            const char* skyboxPath;
        };

        inline constexpr std::array<LevelConfig, 3> LEVEL_CONFIGS{{
            {"The Beginning", "assets/textures/skyboxes/skybox_1.hdr"},
            {"The Deep", "assets/textures/skyboxes/skybox_2.hdr"},
            {"Final Ascent", "assets/textures/skyboxes/skybox_3.hdr"}
        }};

        inline const LevelConfig& getLevelConfig(int levelIndex) {
            if (levelIndex >= 1 && static_cast<size_t>(levelIndex) <= LEVEL_CONFIGS.size()) {
                return LEVEL_CONFIGS[static_cast<size_t>(levelIndex - 1)];
            }
            return LEVEL_CONFIGS[0];
        }
    }
}
