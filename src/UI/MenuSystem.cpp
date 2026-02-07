#include "MenuSystem.h"
#include "imgui.h"
#include "Game.h" // For GameState enum
#include "Config.h"
#include "Settings.h"
#include "AudioSystem.h"

MenuSystem::MenuSystem(GuiSystem& gui, AudioSystem& audio, Callbacks callbacks) 
    : m_gui(gui), m_audio(audio), m_callbacks(callbacks) {
    m_backgroundTexture = std::make_unique<Texture>();
    if (!m_backgroundTexture->loadFromFile("assets/textures/menu_bg.png")) {
        m_backgroundTexture.reset(); // Don't use if failed to load
    }
}

MenuSystem::~MenuSystem() {}

void MenuSystem::render(GameState state, int currentLevel) {
    m_gui.beginFrame();

    switch (state) {
        case GameState::PLAYING:
        case GameState::GAME_OVER:
            // If settings were open when moving to playing, minimize them
            if (m_showSettings && state == GameState::PLAYING) {
                m_showSettings = false;
                m_settingsMinimized = true;
            }
            if (m_callbacks.onRenderHUD) m_callbacks.onRenderHUD();
            break;
        case GameState::MAIN_MENU:
            m_settingsMinimized = false; // Reset minimization state in main menu
            if (!m_showSettings) renderMainMenu(); // Hide main menu buttons if settings are open (optional style choice)
            else {
                // Keep background but not buttons? Or just draw over it. 
                // Let's just draw Main Menu below it.
                renderMainMenu(); 
            }
            // Render new game confirmation dialog if active
            if (m_showNewGameConfirmation) {
                renderNewGameConfirmation();
            }
            break;
        case GameState::PAUSED:
            // If settings were minimized, restore them when pausing
            if (m_settingsMinimized) {
                m_showSettings = true;
                m_settingsMinimized = false;
            }

            if (!m_showSettings) renderPauseMenu();
            else {
                 // Draw pause menu background only?
                 // Simpler: Just render pause menu, settings will be on top.
                 renderPauseMenu();
            }
            break;
        case GameState::QUIT_CONFIRMATION:
            renderQuitConfirmation(currentLevel);
            break;
        case GameState::LEVEL_WIN:
            renderLevelWin(currentLevel);
            break;
        case GameState::GAME_WIN:
            renderGameWin();
            break;
        default:
            break;
    }

    // Global Settings Modal (Always drawn on top)
    if (m_showSettings) {
        renderSettingsMenu(&m_showSettings);
    } 

    m_gui.endFrame();
    m_gui.render();
}

void MenuSystem::renderMainMenu() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);

    float scale = io.DisplaySize.y / Config::UI_REFERENCE_HEIGHT;
    m_gui.getBigFont()->Scale = scale;
    m_gui.getFont()->Scale = scale;

    ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);
    
    // Background Image
    if (m_backgroundTexture) {
        ImVec2 uv0(0, 0);
        ImVec2 uv1(1, 1);
        float screenAspect = io.DisplaySize.x / io.DisplaySize.y;
        float imageAspect = (float)m_backgroundTexture->getWidth() / (float)m_backgroundTexture->getHeight();

        if (screenAspect > imageAspect) {
            float amount = imageAspect / screenAspect;
            uv0.y = 0.5f - (amount * 0.5f);
            uv1.y = 0.5f + (amount * 0.5f);
        } else {
            float amount = screenAspect / imageAspect;
            uv0.x = 0.5f - (amount * 0.5f);
            uv1.x = 0.5f + (amount * 0.5f);
        }
        ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)m_backgroundTexture->ID, ImVec2(0, 0), io.DisplaySize, uv0, uv1);
    } else {
        ImVec2 bottomRight(io.DisplaySize.x, io.DisplaySize.y);
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), bottomRight, ImColor(15, 20, 35));
    }

    Settings& settings = Settings::getInstance();
    
    // Bottom Right Title
    ImGui::PushFont(m_gui.getBigFont());
    const char* title = "BULLET SHIFT";
    ImVec2 titleSize = ImGui::CalcTextSize(title);
    float padding = 50.0f * scale;
    ImGui::SetCursorPosX(io.DisplaySize.x - titleSize.x - padding);
    ImGui::SetCursorPosY(io.DisplaySize.y - titleSize.y - padding);
    ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "%s", title);
    ImGui::PopFont();

    // Bottom Left Menu Options
    float buttonWidth = 220.0f * scale;
    float buttonHeight = 45.0f * scale;
    float leftPadding = 60.0f * scale;
    float bottomPadding = 80.0f * scale;
    float spacing = 10.0f * scale;

    // Calculate total menu height to align from bottom
    int buttonCount = 3 + (settings.progress.lastLevelPlayed > 0 ? 1 : 0);
    float totalHeight = (buttonCount * buttonHeight) + ((buttonCount - 1) * spacing);
    float currentY = io.DisplaySize.y - bottomPadding - totalHeight;

    auto renderMenuButton = [&](const char* label, std::function<void()> onClick) {
        ImGui::SetCursorPosX(leftPadding);
        ImGui::SetCursorPosY(currentY);
        if (ImGui::Button(label, ImVec2(buttonWidth, buttonHeight))) {
            m_audio.playSound("ui_click");
            onClick();
        }
        currentY += buttonHeight + spacing;
    };

    renderMenuButton("START GAME", [&]() {
        if (settings.progress.lastLevelPlayed > 0) {
            m_showNewGameConfirmation = true;
        } else {
            if (m_callbacks.onLoadLevel) m_callbacks.onLoadLevel(1);
        }
    });

    if (settings.progress.lastLevelPlayed > 0) {
        renderMenuButton("CONTINUE", [&]() {
            if (m_callbacks.onLoadLevel) m_callbacks.onLoadLevel(settings.progress.lastLevelPlayed);
        });
    }

    renderMenuButton("SETTINGS", [&]() {
        m_showSettings = true;
    });

    renderMenuButton("EXIT", [&]() {
        if (m_callbacks.onQuitApp) m_callbacks.onQuitApp();
    });

    ImGui::End();
}

void MenuSystem::renderPauseMenu() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);

    float scale = io.DisplaySize.y / Config::UI_REFERENCE_HEIGHT;
    m_gui.getBigFont()->Scale = scale;
    m_gui.getFont()->Scale = scale;

    ImGui::Begin("Pause Menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground);
    
    // Full screen overlay
    ImVec2 bottomRight(io.DisplaySize.x, io.DisplaySize.y);
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), bottomRight, ImColor(10, 10, 10, 150)); 

    // Bottom Right Title
    ImGui::PushFont(m_gui.getBigFont());
    const char* title = "PAUSED";
    ImVec2 titleSize = ImGui::CalcTextSize(title);
    float padding = 50.0f * scale;
    ImGui::SetCursorPosX(io.DisplaySize.x - titleSize.x - padding);
    ImGui::SetCursorPosY(io.DisplaySize.y - titleSize.y - padding);
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s", title); // Gold color for pause
    ImGui::PopFont();

    // Bottom Left Menu Options (Mirroring Main Menu logic)
    float buttonWidth = 220.0f * scale;
    float buttonHeight = 45.0f * scale;
    float leftPadding = 60.0f * scale;
    float bottomPadding = 80.0f * scale;
    float spacing = 10.0f * scale;

    int buttonCount = 3; 
    float totalHeight = (buttonCount * buttonHeight) + ((buttonCount - 1) * spacing);
    float currentY = io.DisplaySize.y - bottomPadding - totalHeight;

    auto renderMenuButton = [&](const char* label, std::function<void()> onClick) {
        ImGui::SetCursorPosX(leftPadding);
        ImGui::SetCursorPosY(currentY);
        if (ImGui::Button(label, ImVec2(buttonWidth, buttonHeight))) {
            m_audio.playSound("ui_click");
            onClick();
        }
        currentY += buttonHeight + spacing;
    };

    renderMenuButton("RESUME", [&]() {
        if (m_callbacks.onResume) m_callbacks.onResume();
    });

    renderMenuButton("SETTINGS", [&]() {
        m_showSettings = true;
    });

    renderMenuButton("EXIT TO MENU", [&]() {
        if (m_callbacks.onExitToMenu) m_callbacks.onExitToMenu();
    });

    ImGui::End();
}

void MenuSystem::renderQuitConfirmation(int /*currentLevel*/) {
    ImGuiIO& io = ImGui::GetIO();

    float scale = io.DisplaySize.y / Config::UI_REFERENCE_HEIGHT;
    m_gui.getBigFont()->Scale = scale;
    m_gui.getFont()->Scale = scale;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("Quit Background", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground);
    ImVec2 bottomRight(io.DisplaySize.x, io.DisplaySize.y);
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), bottomRight, ImColor(10, 10, 10, 150));
    ImGui::End();

    ImGui::OpenPopup("Quit Confirmation");
    ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal("Quit Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
        ImGui::Text("\n  Are you sure you want to exit?  \n\n");
        
        if (ImGui::Button("YES", ImVec2(120 * scale, 40 * scale))) {
            m_audio.playSound("ui_click");
            if (m_callbacks.onQuitApp) m_callbacks.onQuitApp();
        }
        ImGui::SameLine();
        if (ImGui::Button("NO", ImVec2(120 * scale, 40 * scale))) {
            m_audio.playSound("ui_cancel");
            if (m_callbacks.onResume) m_callbacks.onResume(); // Resume or go back using the same callback
            ImGui::CloseCurrentPopup();
        }
        ImGui::End();
    }
}

void MenuSystem::renderLevelWin(int currentLevel) {
    ImGuiIO& io = ImGui::GetIO();

    float scale = io.DisplaySize.y / Config::UI_REFERENCE_HEIGHT;
    m_gui.getBigFont()->Scale = scale;
    m_gui.getFont()->Scale = scale;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("Level Win", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground);
    ImVec2 bottomRight(io.DisplaySize.x, io.DisplaySize.y);
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), bottomRight, ImColor(10, 30, 10, 150));

    ImGui::PushFont(m_gui.getBigFont());
    const char* msg = "LEVEL COMPLETE!";
    float textWidth = ImGui::CalcTextSize(msg).x;
    ImGui::SetCursorPosX((io.DisplaySize.x - textWidth) * 0.5f);
    ImGui::SetCursorPosY(io.DisplaySize.y * 0.3f);
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s", msg);
    ImGui::PopFont();

    float buttonWidth = 250.0f * scale;
    ImGui::SetCursorPosX((io.DisplaySize.x - buttonWidth) * 0.5f);
    ImGui::SetCursorPosY(io.DisplaySize.y * 0.5f);
    if (ImGui::Button("NEXT LEVEL", ImVec2(buttonWidth, 60 * scale))) {
        m_audio.playSound("ui_click");
        if (m_callbacks.onLoadLevel) m_callbacks.onLoadLevel(currentLevel + 1);
    }
    
    ImGui::SetCursorPosX((io.DisplaySize.x - buttonWidth) * 0.5f);
    if (ImGui::Button("MAIN MENU", ImVec2(buttonWidth, 60 * scale))) {
        m_audio.playSound("ui_click");
        if (m_callbacks.onExitToMenu) m_callbacks.onExitToMenu();
    }

    ImGui::End();
}

void MenuSystem::renderGameWin() {
    ImGuiIO& io = ImGui::GetIO();

    float scale = io.DisplaySize.y / Config::UI_REFERENCE_HEIGHT;
    m_gui.getBigFont()->Scale = scale;
    m_gui.getFont()->Scale = scale;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("Game Win", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground);
    ImVec2 bottomRight(io.DisplaySize.x, io.DisplaySize.y);
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), bottomRight, ImColor(10, 10, 30, 150));

    ImGui::PushFont(m_gui.getBigFont());
    const char* msg = "YOU WIN!";
    float textWidth = ImGui::CalcTextSize(msg).x;
    ImGui::SetCursorPosX((io.DisplaySize.x - textWidth) * 0.5f);
    ImGui::SetCursorPosY(io.DisplaySize.y * 0.3f);
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", msg);
    ImGui::PopFont();
    
    // Credits
    ImGui::PushFont(m_gui.getFont());
    const char* credits = "Made by Agustín Motta";
    float creditWidth = ImGui::CalcTextSize(credits).x;
    ImGui::SetCursorPosX((io.DisplaySize.x - creditWidth) * 0.5f);
    ImGui::SetCursorPosY(io.DisplaySize.y * 0.45f);
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "%s", credits);
    ImGui::PopFont();

    float buttonWidth = 250.0f * scale;
    ImGui::SetCursorPosX((io.DisplaySize.x - buttonWidth) * 0.5f);
    ImGui::SetCursorPosY(io.DisplaySize.y * 0.7f);
    if (ImGui::Button("MAIN MENU", ImVec2(buttonWidth, 60 * scale))) {
        if (m_callbacks.onExitToMenu) m_callbacks.onExitToMenu();
    }

    ImGui::End();
}

void MenuSystem::renderGameOver(int currentLevel) {
    ImGuiIO& io = ImGui::GetIO();
    float scale = io.DisplaySize.y / Config::UI_REFERENCE_HEIGHT;
    m_gui.getBigFont()->Scale = scale;
    m_gui.getFont()->Scale = scale;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("Game Over Overlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground);
    ImVec2 bottomRight(io.DisplaySize.x, io.DisplaySize.y);
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), bottomRight, ImColor(30, 10, 10, 150));
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("Game Over Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);

    ImGui::PushFont(m_gui.getBigFont());
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "GAME OVER");
    ImGui::PopFont();
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 10 * scale));
    
    if (ImGui::Button("RETRY", ImVec2(200 * scale, 50 * scale))) {
        m_audio.playSound("ui_click");
        if (m_callbacks.onLoadLevel) m_callbacks.onLoadLevel(currentLevel);
    }
    
    if (ImGui::Button("MAIN MENU", ImVec2(200 * scale, 50 * scale))) {
        m_audio.playSound("ui_click");
        if (m_callbacks.onExitToMenu) m_callbacks.onExitToMenu();
    }
    ImGui::End(); 
}

void MenuSystem::renderSettingsMenu(bool* pOpen) {
    if (!pOpen || !*pOpen) return;

    ImGuiIO& io = ImGui::GetIO();
    float scale = io.DisplaySize.y / Config::UI_REFERENCE_HEIGHT;
    
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(550 * scale, 450 * scale), ImGuiCond_Appearing);
    ImGui::SetNextWindowBgAlpha(0.85f); // Make settings window semi-transparent

    if (ImGui::Begin("Settings", pOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking)) {
        
        Settings& settings = Settings::getInstance();
        bool changed = false;

        if (ImGui::BeginTabBar("SettingsTabs")) {
            
            // ===== AUDIO TAB =====
            if (ImGui::BeginTabItem("Audio")) {
                ImGui::Dummy(ImVec2(0, 10 * scale));
                
                float masterVol = settings.audio.masterVolume * 100.0f;
                if (ImGui::SliderFloat("Master Volume", &masterVol, 0.0f, 100.0f, "%.0f%%")) {
                    settings.audio.masterVolume = masterVol / 100.0f;
                    m_audio.setMasterVolume(settings.audio.masterVolume);
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) changed = true;

                float musicVol = settings.audio.musicVolume * 100.0f;
                if (ImGui::SliderFloat("Music Volume", &musicVol, 0.0f, 100.0f, "%.0f%%")) {
                     settings.audio.musicVolume = musicVol / 100.0f;
                     m_audio.setMusicVolume(settings.audio.musicVolume); 
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) changed = true;

                float sfxVol = settings.audio.sfxVolume * 100.0f;
                if (ImGui::SliderFloat("SFX Volume", &sfxVol, 0.0f, 100.0f, "%.0f%%")) {
                    settings.audio.sfxVolume = sfxVol / 100.0f;
                    m_audio.setSfxVolume(settings.audio.sfxVolume);
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) changed = true;

                ImGui::EndTabItem();
            }
            
            // ===== VIDEO TAB =====
            if (ImGui::BeginTabItem("Video")) {
                ImGui::Dummy(ImVec2(0, 10 * scale));
                
                ImGui::Text("Display");
                ImGui::Separator();
                changed |= ImGui::Checkbox("Fullscreen", &settings.window.fullscreen);
                changed |= ImGui::Checkbox("VSync", &settings.window.vsync);
                
                ImGui::Dummy(ImVec2(0, 10 * scale));
                ImGui::Text("Graphics Quality");
                ImGui::Separator();

                const char* presets[] = { "Low", "Medium", "High", "Custom" };
                if (ImGui::Combo("Quality Preset", &settings.graphics.qualityPreset, presets, IM_ARRAYSIZE(presets))) {
                    changed = true;
                    if (settings.graphics.qualityPreset == 0) { // Low
                         settings.graphics.anisotropicLevel = 2;
                         settings.window.msaaSamples = 2;
                         settings.graphics.gammaCorrection = false;
                    } else if (settings.graphics.qualityPreset == 1) { // Medium
                         settings.graphics.anisotropicLevel = 8;
                         settings.window.msaaSamples = 4;
                         settings.graphics.gammaCorrection = true;
                    } else if (settings.graphics.qualityPreset == 2) { // High
                         settings.graphics.anisotropicLevel = 16;
                         settings.window.msaaSamples = 8;
                         settings.graphics.gammaCorrection = true;
                    } 
                }

                int aniso = settings.graphics.anisotropicLevel;
                if (ImGui::SliderInt("Anisotropic (Restart)", &aniso, 1, 16)) {
                     settings.graphics.anisotropicLevel = aniso;
                     settings.graphics.qualityPreset = 3; 
                     changed = true;
                }

                int msaa = settings.window.msaaSamples;
                if (ImGui::SliderInt("MSAA Samples (Restart)", &msaa, 0, 8)) {
                    settings.window.msaaSamples = msaa;
                    settings.graphics.qualityPreset = 3;
                    changed = true;
                }

                if (ImGui::Checkbox("Gamma Correction (Restart)", &settings.graphics.gammaCorrection)) {
                    settings.graphics.qualityPreset = 3;
                    changed = true;
                }

                ImGui::Dummy(ImVec2(0, 10 * scale));
                ImGui::Text("Post-Processing");
                ImGui::Separator();
                
                changed |= ImGui::Checkbox("Bloom", &settings.graphics.bloomEnabled);
                if (settings.graphics.bloomEnabled) {
                    changed |= ImGui::SliderFloat("Bloom Intensity", &settings.graphics.bloomIntensity, 0.0f, 2.0f);
                    changed |= ImGui::SliderFloat("Bloom Threshold", &settings.graphics.bloomThreshold, 0.5f, 2.0f);
                }
                
                changed |= ImGui::Checkbox("Screen-space Fog", &settings.graphics.fogEnabled);
                if (settings.graphics.fogEnabled) {
                    changed |= ImGui::SliderFloat("Fog Density", &settings.graphics.fogDensity, 0.0f, 0.1f, "%.3f");
                    changed |= ImGui::ColorEdit3("Fog Color", &settings.graphics.fogColor[0]);
                }
                
                changed |= ImGui::SliderFloat("Exposure", &settings.graphics.exposure, 0.1f, 5.0f);

                changed |= ImGui::SliderFloat("Tech Style Intensity", &settings.graphics.techStyleIntensity, 0.0f, 1.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Adjust the intensity of tech-style edge glow and scan line effects");
                }

                changed |= ImGui::Checkbox("Show FPS", &settings.graphics.showFPS);

                ImGui::EndTabItem();
            }
            
            // ===== INPUT TAB =====
            if (ImGui::BeginTabItem("Controls")) {
                ImGui::Dummy(ImVec2(0, 10 * scale));
                
                ImGui::Text("Mouse");
                ImGui::Separator();
                changed |= ImGui::SliderFloat("Mouse Sensitivity", &settings.input.mouseSensitivity, 0.01f, 1.0f);
                changed |= ImGui::Checkbox("Invert Y-Axis", &settings.input.invertY);

                ImGui::Dummy(ImVec2(0, 10 * scale));
                ImGui::Text("Key Bindings");
                ImGui::Separator();
                
                renderKeyBindingsSection();

                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }

        if (changed) {
            settings.save();
            applySettings();
        }

        ImGui::Dummy(ImVec2(0, 10 * scale));
        if (ImGui::Button("Back", ImVec2(100 * scale, 30 * scale))) {
            m_audio.playSound("ui_cancel");
            *pOpen = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Save", ImVec2(100 * scale, 30 * scale))) {
            m_audio.playSound("ui_click");
            applySettings();
            *pOpen = false;
        }

        ImGui::End();
    }

    // Capture closing via the window 'X' button
    if (pOpen && !*pOpen) {
        m_audio.playSound("ui_cancel");
    }
}

void MenuSystem::renderNewGameConfirmation() {
    ImGuiIO& io = ImGui::GetIO();

    float scale = io.DisplaySize.y / Config::UI_REFERENCE_HEIGHT;
    m_gui.getBigFont()->Scale = scale;
    m_gui.getFont()->Scale = scale;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("New Game Background", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground);
    ImVec2 bottomRight(io.DisplaySize.x, io.DisplaySize.y);
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), bottomRight, ImColor(10, 10, 10, 150));
    ImGui::End();

    ImGui::OpenPopup("New Game Warning");
    ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal("New Game Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
        ImGui::PushFont(m_gui.getBigFont());
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "WARNING");
        ImGui::PopFont();
        
        ImGui::Spacing();
        ImGui::Text("\n  Starting a new game will erase  \n");
        ImGui::Text("  your current saved progress!  \n\n");
        
        if (ImGui::Button("START NEW GAME", ImVec2(180 * scale, 40 * scale))) {
            m_audio.playSound("ui_click");
            // Reset progress and start from level 1
            Settings& settings = Settings::getInstance();
            settings.progress.lastLevelPlayed = 0;
            settings.save();
            if (m_callbacks.onLoadLevel) m_callbacks.onLoadLevel(1);
            m_showNewGameConfirmation = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("CANCEL", ImVec2(120 * scale, 40 * scale))) {
            m_audio.playSound("ui_cancel");
            m_showNewGameConfirmation = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::End();
    }
}

void MenuSystem::applySettings() {
    Settings& settings = Settings::getInstance();
    m_audio.setMasterVolume(settings.audio.masterVolume);
    m_audio.setMusicVolume(settings.audio.musicVolume); 
    m_audio.setSfxVolume(settings.audio.sfxVolume);

    if (m_callbacks.onSettingsChanged) {
        m_callbacks.onSettingsChanged();
    }
}

const char* MenuSystem::getKeyName(int keyCode) {
    switch (keyCode) {
        case GLFW_KEY_SPACE: return "Space";
        case GLFW_KEY_APOSTROPHE: return "'";
        case GLFW_KEY_COMMA: return ",";
        case GLFW_KEY_MINUS: return "-";
        case GLFW_KEY_PERIOD: return ".";
        case GLFW_KEY_SLASH: return "/";
        case GLFW_KEY_0: return "0";
        case GLFW_KEY_1: return "1";
        case GLFW_KEY_2: return "2";
        case GLFW_KEY_3: return "3";
        case GLFW_KEY_4: return "4";
        case GLFW_KEY_5: return "5";
        case GLFW_KEY_6: return "6";
        case GLFW_KEY_7: return "7";
        case GLFW_KEY_8: return "8";
        case GLFW_KEY_9: return "9";
        case GLFW_KEY_SEMICOLON: return ";";
        case GLFW_KEY_EQUAL: return "=";
        case GLFW_KEY_A: return "A";
        case GLFW_KEY_B: return "B";
        case GLFW_KEY_C: return "C";
        case GLFW_KEY_D: return "D";
        case GLFW_KEY_E: return "E";
        case GLFW_KEY_F: return "F";
        case GLFW_KEY_G: return "G";
        case GLFW_KEY_H: return "H";
        case GLFW_KEY_I: return "I";
        case GLFW_KEY_J: return "J";
        case GLFW_KEY_K: return "K";
        case GLFW_KEY_L: return "L";
        case GLFW_KEY_M: return "M";
        case GLFW_KEY_N: return "N";
        case GLFW_KEY_O: return "O";
        case GLFW_KEY_P: return "P";
        case GLFW_KEY_Q: return "Q";
        case GLFW_KEY_R: return "R";
        case GLFW_KEY_S: return "S";
        case GLFW_KEY_T: return "T";
        case GLFW_KEY_U: return "U";
        case GLFW_KEY_V: return "V";
        case GLFW_KEY_W: return "W";
        case GLFW_KEY_X: return "X";
        case GLFW_KEY_Y: return "Y";
        case GLFW_KEY_Z: return "Z";
        case GLFW_KEY_LEFT_BRACKET: return "[";
        case GLFW_KEY_BACKSLASH: return "\\";
        case GLFW_KEY_RIGHT_BRACKET: return "]";
        case GLFW_KEY_GRAVE_ACCENT: return "`";
        case GLFW_KEY_ESCAPE: return "Escape";
        case GLFW_KEY_ENTER: return "Enter";
        case GLFW_KEY_TAB: return "Tab";
        case GLFW_KEY_BACKSPACE: return "Backspace";
        case GLFW_KEY_INSERT: return "Insert";
        case GLFW_KEY_DELETE: return "Delete";
        case GLFW_KEY_RIGHT: return "Right";
        case GLFW_KEY_LEFT: return "Left";
        case GLFW_KEY_DOWN: return "Down";
        case GLFW_KEY_UP: return "Up";
        case GLFW_KEY_PAGE_UP: return "Page Up";
        case GLFW_KEY_PAGE_DOWN: return "Page Down";
        case GLFW_KEY_HOME: return "Home";
        case GLFW_KEY_END: return "End";
        case GLFW_KEY_CAPS_LOCK: return "Caps Lock";
        case GLFW_KEY_SCROLL_LOCK: return "Scroll Lock";
        case GLFW_KEY_NUM_LOCK: return "Num Lock";
        case GLFW_KEY_PRINT_SCREEN: return "Print Screen";
        case GLFW_KEY_PAUSE: return "Pause";
        case GLFW_KEY_F1: return "F1";
        case GLFW_KEY_F2: return "F2";
        case GLFW_KEY_F3: return "F3";
        case GLFW_KEY_F4: return "F4";
        case GLFW_KEY_F5: return "F5";
        case GLFW_KEY_F6: return "F6";
        case GLFW_KEY_F7: return "F7";
        case GLFW_KEY_F8: return "F8";
        case GLFW_KEY_F9: return "F9";
        case GLFW_KEY_F10: return "F10";
        case GLFW_KEY_F11: return "F11";
        case GLFW_KEY_F12: return "F12";
        case GLFW_KEY_LEFT_SHIFT: return "Left Shift";
        case GLFW_KEY_LEFT_CONTROL: return "Left Ctrl";
        case GLFW_KEY_LEFT_ALT: return "Left Alt";
        case GLFW_KEY_LEFT_SUPER: return "Left Super";
        case GLFW_KEY_RIGHT_SHIFT: return "Right Shift";
        case GLFW_KEY_RIGHT_CONTROL: return "Right Ctrl";
        case GLFW_KEY_RIGHT_ALT: return "Right Alt";
        case GLFW_KEY_RIGHT_SUPER: return "Right Super";
        case GLFW_KEY_MENU: return "Menu";
        default: return "Unknown";
    }
}

void MenuSystem::renderKeyBindingsSection() {
    Settings& settings = Settings::getInstance();
    ImGuiIO& io = ImGui::GetIO();
    float scale = io.DisplaySize.y / Config::UI_REFERENCE_HEIGHT;
    
    // Helper lambda to render a key binding row
    auto renderKeyRow = [&](const char* label, int* keyBinding) {
        ImGui::Text("%s", label);
        ImGui::SameLine(180 * scale);
        ImGui::Text("[%s]", getKeyName(*keyBinding));
        ImGui::SameLine(300 * scale);
        
        std::string buttonLabel = "Rebind##" + std::string(label);
        if (ImGui::Button(buttonLabel.c_str(), ImVec2(80 * scale, 0))) {
            m_capturingKey = true;
            m_captureActionName = label;
            m_captureTarget = keyBinding;
            ImGui::OpenPopup("Press a Key");
        }
    };
    
    // Render all key bindings
    renderKeyRow("Move Forward", &settings.keybinds.moveForward);
    renderKeyRow("Move Backward", &settings.keybinds.moveBackward);
    renderKeyRow("Move Left", &settings.keybinds.moveLeft);
    renderKeyRow("Move Right", &settings.keybinds.moveRight);
    renderKeyRow("Jump", &settings.keybinds.jump);
    renderKeyRow("Dash", &settings.keybinds.dash);
    renderKeyRow("Reload", &settings.keybinds.reload);
    renderKeyRow("Switch Weapon", &settings.keybinds.switchWeapon);
    renderKeyRow("Interact", &settings.keybinds.interact);
    renderKeyRow("Bullet Time", &settings.keybinds.bulletTime);
    
    // Key capture modal
    ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal("Press a Key", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::Text("Press a key to bind to: %s", m_captureActionName.c_str());
        ImGui::Text("\nPress ESC to cancel\n\n");
        
        // Check for key presses
        for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
            if (ImGui::IsKeyPressed((ImGuiKey)key, false)) {
                // Map ImGui key back to GLFW key
                // ImGui uses its own key enum, we need to detect raw keypresses
                // Actually, we should check GLFW directly for this
            }
        }
        
        // Use GLFW to detect key presses since we need GLFW key codes
        // We'll iterate through common keys and check if any are pressed
        bool keyFound = false;
        int pressedKey = -1;
        
        // Check all possible GLFW keys
        for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
            if (key == GLFW_KEY_ESCAPE) continue; // ESC is for cancel
            
            // We need to check via ImGui since we don't have direct GLFW window access here
            // Use ImGui's key detection which maps to GLFW internally
            ImGuiKey imguiKey = (ImGuiKey)(key);
            if (ImGui::IsKeyPressed(imguiKey, false)) {
                pressedKey = key;
                keyFound = true;
                break;
            }
        }
        
        // Manual check for common keys using ImGui named keys
        if (!keyFound) {
            struct KeyMapping { ImGuiKey imguiKey; int glfwKey; };
            static const KeyMapping keyMappings[] = {
                {ImGuiKey_A, GLFW_KEY_A}, {ImGuiKey_B, GLFW_KEY_B}, {ImGuiKey_C, GLFW_KEY_C},
                {ImGuiKey_D, GLFW_KEY_D}, {ImGuiKey_E, GLFW_KEY_E}, {ImGuiKey_F, GLFW_KEY_F},
                {ImGuiKey_G, GLFW_KEY_G}, {ImGuiKey_H, GLFW_KEY_H}, {ImGuiKey_I, GLFW_KEY_I},
                {ImGuiKey_J, GLFW_KEY_J}, {ImGuiKey_K, GLFW_KEY_K}, {ImGuiKey_L, GLFW_KEY_L},
                {ImGuiKey_M, GLFW_KEY_M}, {ImGuiKey_N, GLFW_KEY_N}, {ImGuiKey_O, GLFW_KEY_O},
                {ImGuiKey_P, GLFW_KEY_P}, {ImGuiKey_Q, GLFW_KEY_Q}, {ImGuiKey_R, GLFW_KEY_R},
                {ImGuiKey_S, GLFW_KEY_S}, {ImGuiKey_T, GLFW_KEY_T}, {ImGuiKey_U, GLFW_KEY_U},
                {ImGuiKey_V, GLFW_KEY_V}, {ImGuiKey_W, GLFW_KEY_W}, {ImGuiKey_X, GLFW_KEY_X},
                {ImGuiKey_Y, GLFW_KEY_Y}, {ImGuiKey_Z, GLFW_KEY_Z},
                {ImGuiKey_0, GLFW_KEY_0}, {ImGuiKey_1, GLFW_KEY_1}, {ImGuiKey_2, GLFW_KEY_2},
                {ImGuiKey_3, GLFW_KEY_3}, {ImGuiKey_4, GLFW_KEY_4}, {ImGuiKey_5, GLFW_KEY_5},
                {ImGuiKey_6, GLFW_KEY_6}, {ImGuiKey_7, GLFW_KEY_7}, {ImGuiKey_8, GLFW_KEY_8},
                {ImGuiKey_9, GLFW_KEY_9},
                {ImGuiKey_Space, GLFW_KEY_SPACE}, {ImGuiKey_Tab, GLFW_KEY_TAB},
                {ImGuiKey_LeftShift, GLFW_KEY_LEFT_SHIFT}, {ImGuiKey_RightShift, GLFW_KEY_RIGHT_SHIFT},
                {ImGuiKey_LeftCtrl, GLFW_KEY_LEFT_CONTROL}, {ImGuiKey_RightCtrl, GLFW_KEY_RIGHT_CONTROL},
                {ImGuiKey_LeftAlt, GLFW_KEY_LEFT_ALT}, {ImGuiKey_RightAlt, GLFW_KEY_RIGHT_ALT},
                {ImGuiKey_Enter, GLFW_KEY_ENTER}, {ImGuiKey_Backspace, GLFW_KEY_BACKSPACE},
                {ImGuiKey_Insert, GLFW_KEY_INSERT}, {ImGuiKey_Delete, GLFW_KEY_DELETE},
                {ImGuiKey_Home, GLFW_KEY_HOME}, {ImGuiKey_End, GLFW_KEY_END},
                {ImGuiKey_PageUp, GLFW_KEY_PAGE_UP}, {ImGuiKey_PageDown, GLFW_KEY_PAGE_DOWN},
                {ImGuiKey_LeftArrow, GLFW_KEY_LEFT}, {ImGuiKey_RightArrow, GLFW_KEY_RIGHT},
                {ImGuiKey_UpArrow, GLFW_KEY_UP}, {ImGuiKey_DownArrow, GLFW_KEY_DOWN},
                {ImGuiKey_F1, GLFW_KEY_F1}, {ImGuiKey_F2, GLFW_KEY_F2}, {ImGuiKey_F3, GLFW_KEY_F3},
                {ImGuiKey_F4, GLFW_KEY_F4}, {ImGuiKey_F5, GLFW_KEY_F5}, {ImGuiKey_F6, GLFW_KEY_F6},
                {ImGuiKey_F7, GLFW_KEY_F7}, {ImGuiKey_F8, GLFW_KEY_F8}, {ImGuiKey_F9, GLFW_KEY_F9},
                {ImGuiKey_F10, GLFW_KEY_F10}, {ImGuiKey_F11, GLFW_KEY_F11}, {ImGuiKey_F12, GLFW_KEY_F12},
            };
            
            for (const auto& mapping : keyMappings) {
                if (ImGui::IsKeyPressed(mapping.imguiKey, false)) {
                    pressedKey = mapping.glfwKey;
                    keyFound = true;
                    break;
                }
            }
        }
        
        if (keyFound && m_captureTarget) {
            *m_captureTarget = pressedKey;
            settings.save();
            m_capturingKey = false;
            m_captureTarget = nullptr;
            ImGui::CloseCurrentPopup();
        }
        
        // Cancel with ESC
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            m_capturingKey = false;
            m_captureTarget = nullptr;
            ImGui::CloseCurrentPopup();
        }
        
        if (ImGui::Button("Cancel", ImVec2(100 * scale, 30 * scale))) {
            m_audio.playSound("ui_cancel");
            m_capturingKey = false;
            m_captureTarget = nullptr;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}
