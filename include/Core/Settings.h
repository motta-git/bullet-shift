#pragma once

#include <string>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

// Key bindings for rebindable controls
struct KeyBindings {
    int moveForward = GLFW_KEY_W;
    int moveBackward = GLFW_KEY_S;
    int moveLeft = GLFW_KEY_A;
    int moveRight = GLFW_KEY_D;
    int jump = GLFW_KEY_SPACE;
    int dash = GLFW_KEY_LEFT_SHIFT;
    int reload = GLFW_KEY_R;
    int switchWeapon = GLFW_KEY_Q;
    int interact = GLFW_KEY_E;
    int bulletTime = GLFW_KEY_F;
};

struct WindowSettings {
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    bool vsync = true;
    int msaaSamples = 4;
};

struct AudioSettings {
    float masterVolume = 1.0f;
    float musicVolume = 1.0f;
    float sfxVolume = 1.0f;
};

struct GraphicSettings {
    int qualityPreset = 2; // 0:Low, 1:Medium, 2:High, 3:Custom
    int anisotropicLevel = 16; // 1, 2, 4, 8, 16
    bool gammaCorrection = true;
    float techStyleIntensity = 0.6f; // 0.0 = off, 1.0 = full tech effect
    bool showFPS = false;

    // Post-processing
    bool bloomEnabled = true;
    float bloomThreshold = 1.0f;
    float bloomIntensity = 0.5f;
    bool colorGradingEnabled = true;
    float exposure = 1.0f;
    bool fogEnabled = true;
    float fogDensity = 0.015f;
    glm::vec3 fogColor = glm::vec3(0.05f, 0.05f, 0.08f);
};

struct InputSettings {
    float mouseSensitivity = 0.1f;
    bool invertY = false;
};

struct GameProgress {
    int lastLevelPlayed = 0; // 0 means no progress
};

class Settings {
public:
    static Settings& getInstance();

    WindowSettings window;
    AudioSettings audio;
    GraphicSettings graphics;
    InputSettings input;
    GameProgress progress;
    KeyBindings keybinds;

    // Load settings from file (returns true if successful)
    bool load(const std::string& filepath = "settings.ini");
    
    // Save settings to file
    void save(const std::string& filepath = "settings.ini") const;

private:
    Settings();
    ~Settings() = default;
    
    // Delete copy constructor and assignment operator to ensure Singleton
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;
};
