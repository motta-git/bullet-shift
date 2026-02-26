#include "DevConsole.h"
#include "Game.h"
#include "Config.h"
#include "Settings.h"

#include <imgui.h>
#include <algorithm>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <cstring>

DevConsole::DevConsole()
    : m_isOpen(false),
      m_godMode(false),
      m_historyPos(-1),
      m_scrollToBottom(false),
      m_reclaimFocus(false) {
    memset(m_inputBuf, 0, sizeof(m_inputBuf));
    print("Developer Console initialized. Type 'help' for commands.");
}

void DevConsole::toggle() {
    m_isOpen = !m_isOpen;
    if (m_isOpen) {
        m_reclaimFocus = true;
    }
}

void DevConsole::close() {
    m_isOpen = false;
}

void DevConsole::print(const std::string& msg) {
    m_log.push_back(msg);
    m_fullLog += msg + "\n";
    m_scrollToBottom = true;
}

void DevConsole::execCommand(const std::string& rawCmd, Game& game) {
    // Print the command with a > prefix
    print("> " + rawCmd);

    // Add to history
    // Remove duplicate if the same command was the last one
    if (m_cmdHistory.empty() || m_cmdHistory.back() != rawCmd) {
        m_cmdHistory.push_back(rawCmd);
    }
    m_historyPos = -1;

    // Tokenize
    std::istringstream iss(rawCmd);
    std::string command;
    iss >> command;

    // Convert command to lowercase
    std::transform(command.begin(), command.end(), command.begin(), ::tolower);

    if (command == "help") {
        print("Available commands:");
        print("  map <level>  - Load a level (e.g. map 1)");
        print("  god          - Toggle god mode (invincibility)");
        print("  clear        - Clear the console");
        print("  help         - Show this help");
    } else if (command == "clear") {
        m_log.clear();
        m_fullLog.clear();
    } else if (command == "bind") {
        std::string subCmd;
        iss >> subCmd;
        if (subCmd == "console") {
            int newKey;
            if (iss >> newKey) {
                Settings::getInstance().keybinds.consoleToggle = newKey;
                Settings::getInstance().save();
                print("Console bound to key code: " + std::to_string(newKey));
            } else {
                print("Usage: bind console <GLFW_KEY_CODE>");
            }
        } else {
            print("Usage: bind <command> <args>");
            print("Currently supported: bind console <key_code>");
        }
    } else if (command == "god") {
        m_godMode = !m_godMode;
        game.player.setGodMode(m_godMode);
        if (m_godMode) {
            print("God mode ON - You are invincible!");
            // Heal to full when enabling god mode
            game.player.heal(game.player.getMaxHealth());
        } else {
            print("God mode OFF");
        }
    } else if (command == "map") {
        std::string levelArg;
        iss >> levelArg;
        if (levelArg.empty()) {
            // List available levels
            print("Usage: map <level_number>");
            print("Available levels:");
            try {
                std::vector<int> detected;
                for (const auto& entry : std::filesystem::directory_iterator("assets/levels")) {
                    if (entry.path().extension() == ".glb") {
                        std::string filename = entry.path().stem().string();
                        if (filename.substr(0, 6) == "level_") {
                            try {
                                detected.push_back(std::stoi(filename.substr(6)));
                            } catch (...) {}
                        }
                    }
                }
                std::sort(detected.begin(), detected.end());
                for (int lvl : detected) {
                    if (lvl <= (int)Config::Levels::LEVEL_CONFIGS.size()) {
                        const auto& cfg = Config::Levels::getLevelConfig(lvl);
                        print("  " + std::to_string(lvl) + " - " + cfg.name);
                    } else {
                        print("  " + std::to_string(lvl) + " - Custom");
                    }
                }
            } catch (...) {
                print("Error reading levels directory.");
            }
        } else {
            try {
                int level = std::stoi(levelArg);
                print("Loading level " + std::to_string(level) + "...");
                game.loadLevel(level);
                // Close console after map change
                m_isOpen = false;
            } catch (...) {
                print("Invalid level number: " + levelArg);
            }
        }
    } else {
        print("Unknown command: '" + command + "'. Type 'help' for available commands.");
    }
}

// ImGui text edit callback for command history navigation
struct ConsoleCallbackData {
    DevConsole* console;
    std::vector<std::string>* cmdHistory;
    int* historyPos;
};

int DevConsole::textEditCallback(ImGuiInputTextCallbackData* cbData) {
    ConsoleCallbackData* userData = static_cast<ConsoleCallbackData*>(cbData->UserData);

    if (cbData->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
        auto& history = *userData->cmdHistory;
        int& pos = *userData->historyPos;
        const int prevPos = pos;

        if (cbData->EventKey == ImGuiKey_UpArrow) {
            if (pos == -1) {
                pos = (int)history.size() - 1;
            } else if (pos > 0) {
                pos--;
            }
        } else if (cbData->EventKey == ImGuiKey_DownArrow) {
            if (pos != -1) {
                if (++pos >= (int)history.size()) {
                    pos = -1;
                }
            }
        }

        if (prevPos != pos) {
            const char* historyStr = (pos >= 0) ? history[pos].c_str() : "";
            cbData->DeleteChars(0, cbData->BufTextLen);
            cbData->InsertChars(0, historyStr);
        }
    }

    return 0;
}

void DevConsole::render(Game& game) {
    if (!m_isOpen) return;

    ImGuiIO& io = ImGui::GetIO();
    float scale = io.DisplaySize.y / Config::UI_REFERENCE_HEIGHT;

    // Default window size and position (Valve-style)
    ImGui::SetNextWindowSize(ImVec2(640 * scale, 480 * scale), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(50 * scale, 50 * scale), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;

    // Style: dark semi-transparent blue background to match game theme
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.1f, 0.2f, 0.4f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.15f, 0.35f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.08f, 0.14f, 0.94f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.6f, 1.0f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // White text
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.05f, 0.05f, 0.1f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.06f, 0.08f, 0.14f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.2f, 0.6f, 1.0f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

    if (ImGui::Begin("Console", &m_isOpen, flags)) {
        // Log region (resizable/selectable multiline text)
        float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing() + 5 * scale;
        
        // Use InputTextMultiline with read-only flag for easy copying
        ImGuiInputTextFlags logFlags = ImGuiInputTextFlags_ReadOnly;
        
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0)); // Transparent frame for log
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // Ensure log text is white
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        
        ImGui::InputTextMultiline("##ConsoleLog", (char*)m_fullLog.c_str(), m_fullLog.size(), ImVec2(-FLT_MIN, -footerHeight), logFlags);
        
        if (m_scrollToBottom) {
            ImGui::SetScrollHereY(1.0f);
            m_scrollToBottom = false;
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        // Separator line
        ImGui::Separator();

        // Input line
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // White for input

        ConsoleCallbackData cbUserData;
        cbUserData.console = this;
        cbUserData.cmdHistory = &m_cmdHistory;
        cbUserData.historyPos = &m_historyPos;

        ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue
                                       | ImGuiInputTextFlags_CallbackHistory;

        ImGui::AlignTextToFramePadding();
        ImGui::Text("]");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##ConsoleInput", m_inputBuf, sizeof(m_inputBuf), inputFlags, textEditCallback, &cbUserData)) {
            std::string cmd(m_inputBuf);
            // Trim whitespace
            size_t start = cmd.find_first_not_of(" \t");
            size_t end = cmd.find_last_not_of(" \t");
            if (start != std::string::npos) {
                cmd = cmd.substr(start, end - start + 1);
                execCommand(cmd, game);
            }
            memset(m_inputBuf, 0, sizeof(m_inputBuf));
            m_reclaimFocus = true;
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleColor();

        // Auto-focus on the input field
        if (m_reclaimFocus) {
            ImGui::SetKeyboardFocusHere(-1);
            m_reclaimFocus = false;
        }
    }
    ImGui::End();

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(8);
}
