#include "Settings.h"
#include "Config.h"

#include <fstream>
#include <iostream>
#include <sstream>

Settings& Settings::getInstance() {
    static Settings instance;
    return instance;
}

Settings::Settings() {
    // Initialize default values
    // Using values from Config.h where appropriate
    input.mouseSensitivity = Config::Camera::SENSITIVITY;
    
    // Window defaults
    window.width = 1920;
    window.height = 1080;
    window.fullscreen = true;
    window.vsync = true;
    window.msaaSamples = 8;

    // Audio defaults
    audio.masterVolume = 1.0f;
    audio.musicVolume = 1.0f;
    audio.sfxVolume = 1.0f;

    // Graphics defaults
    graphics.qualityPreset = 2; // High
    graphics.anisotropicLevel = 16;
    graphics.gammaCorrection = true;
    graphics.techStyleIntensity = 0.6f;
    graphics.showFPS = false;
}

bool Settings::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cout << "Settings file not found, using defaults." << std::endl;
        // Optionally save the defaults immediately
        // save(filepath);
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') continue;

        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, '=')) {
            std::string value;
            if (std::getline(iss, value)) {
                
                // Window
                if (key == "window.width") window.width = std::stoi(value);
                else if (key == "window.height") window.height = std::stoi(value);
                else if (key == "window.fullscreen") window.fullscreen = (std::stoi(value) != 0);
                else if (key == "window.vsync") window.vsync = (std::stoi(value) != 0);
                else if (key == "window.msaa") window.msaaSamples = std::stoi(value);
                
                // Audio
                else if (key == "audio.master") audio.masterVolume = std::stof(value);
                else if (key == "audio.music") audio.musicVolume = std::stof(value);
                else if (key == "audio.sfx") audio.sfxVolume = std::stof(value);
                
                // Graphics
                else if (key == "graphics.quality") graphics.qualityPreset = std::stoi(value);
                else if (key == "graphics.aniso") graphics.anisotropicLevel = std::stoi(value);
                else if (key == "graphics.gamma") graphics.gammaCorrection = (std::stoi(value) != 0);
                else if (key == "graphics.techstyle") graphics.techStyleIntensity = std::stof(value);
                else if (key == "graphics.showfps") graphics.showFPS = (std::stoi(value) != 0);

                // Input
                else if (key == "input.sensitivity") input.mouseSensitivity = std::stof(value);
                else if (key == "input.inverty") input.invertY = (std::stoi(value) != 0);
                
                // Game Progress
                else if (key == "progress.lastlevel") progress.lastLevelPlayed = std::stoi(value);
                
                // Key Bindings
                else if (key == "keybinds.forward") keybinds.moveForward = std::stoi(value);
                else if (key == "keybinds.backward") keybinds.moveBackward = std::stoi(value);
                else if (key == "keybinds.left") keybinds.moveLeft = std::stoi(value);
                else if (key == "keybinds.right") keybinds.moveRight = std::stoi(value);
                else if (key == "keybinds.jump") keybinds.jump = std::stoi(value);
                else if (key == "keybinds.dash") keybinds.dash = std::stoi(value);
                else if (key == "keybinds.reload") keybinds.reload = std::stoi(value);
                else if (key == "keybinds.switch") keybinds.switchWeapon = std::stoi(value);
                else if (key == "keybinds.interact") keybinds.interact = std::stoi(value);
                else if (key == "keybinds.bullettime") keybinds.bulletTime = std::stoi(value);
            }
        }
    }
    return true;
}

void Settings::save(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to save settings to " << filepath << std::endl;
        return;
    }

    file << "[Window]\n";
    file << "window.width=" << window.width << "\n";
    file << "window.height=" << window.height << "\n";
    file << "window.fullscreen=" << (window.fullscreen ? 1 : 0) << "\n";
    file << "window.vsync=" << (window.vsync ? 1 : 0) << "\n";
    file << "window.msaa=" << window.msaaSamples << "\n";

    file << "\n[Audio]\n";
    file << "audio.master=" << audio.masterVolume << "\n";
    file << "audio.music=" << audio.musicVolume << "\n";
    file << "audio.sfx=" << audio.sfxVolume << "\n";

    file << "\n[Graphics]\n";
    file << "graphics.quality=" << graphics.qualityPreset << "\n";
    file << "graphics.aniso=" << graphics.anisotropicLevel << "\n";
    file << "graphics.gamma=" << (graphics.gammaCorrection ? 1 : 0) << "\n";
    file << "graphics.techstyle=" << graphics.techStyleIntensity << "\n";
    file << "graphics.showfps=" << (graphics.showFPS ? 1 : 0) << "\n";

    file << "\n[Input]\n";
    file << "input.sensitivity=" << input.mouseSensitivity << "\n";
    file << "input.inverty=" << (input.invertY ? 1 : 0) << "\n";
    
    file << "\n[Progress]\n";
    file << "progress.lastlevel=" << progress.lastLevelPlayed << "\n";
    
    file << "\n[KeyBindings]\n";
    file << "keybinds.forward=" << keybinds.moveForward << "\n";
    file << "keybinds.backward=" << keybinds.moveBackward << "\n";
    file << "keybinds.left=" << keybinds.moveLeft << "\n";
    file << "keybinds.right=" << keybinds.moveRight << "\n";
    file << "keybinds.jump=" << keybinds.jump << "\n";
    file << "keybinds.dash=" << keybinds.dash << "\n";
    file << "keybinds.reload=" << keybinds.reload << "\n";
    file << "keybinds.switch=" << keybinds.switchWeapon << "\n";
    file << "keybinds.interact=" << keybinds.interact << "\n";
    file << "keybinds.bullettime=" << keybinds.bulletTime << "\n";
}
