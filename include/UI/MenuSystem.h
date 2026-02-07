#pragma once

#include <functional>
#include <string>
#include "GuiSystem.h"
#include "Renderer/Texture.h"

enum class GameState;
class AudioSystem;

class MenuSystem {
public:
    struct Callbacks {
        std::function<void(int)> onLoadLevel;
        std::function<void()> onResume;
        std::function<void()> onExitToMenu;
        std::function<void()> onQuitApp;
        std::function<void()> onRenderHUD;
        std::function<void()> onSettingsChanged;
    };

    MenuSystem(GuiSystem& gui, AudioSystem& audio, Callbacks callbacks);
    ~MenuSystem();

    void render(GameState state, int currentLevel);
    
    bool isSettingsOpen() const { return m_showSettings; }
    void closeSettings() { m_showSettings = false; }

private:
public: void renderSettingsMenu(bool* pOpen); // Temporarily public or just for internal usage
private:
    void renderMainMenu();
    void renderPauseMenu();
    void renderQuitConfirmation(int currentLevel);
    void renderGameOver(int currentLevel);
    void renderLevelWin(int currentLevel);
    void renderGameWin();
    void renderNewGameConfirmation();
    
    // Settings state
    bool m_showSettings = false;
    bool m_showNewGameConfirmation = false; 
    bool m_settingsMinimized = false;
    
    // Key rebinding state
    bool m_capturingKey = false;
    std::string m_captureActionName;
    int* m_captureTarget = nullptr;
    
    void applySettings(); // helper
    void renderKeyBindingsSection(); // helper for key bindings UI
    static const char* getKeyName(int keyCode); // helper to get human-readable key name

    GuiSystem& m_gui;
    AudioSystem& m_audio;
    Callbacks m_callbacks;

    std::unique_ptr<Texture> m_backgroundTexture;
};

