#pragma once

#include <string>
#include <vector>
#include <functional>

class Game;

class DevConsole {
public:
    DevConsole();
    ~DevConsole() = default;

    // Toggle open/close
    void toggle();
    void close();

    // Render the console via ImGui (call within a GuiSystem frame)
    void render(Game& game);

    // Print a message to the console log
    void print(const std::string& msg);

    // State queries
    bool isOpen() const { return m_isOpen; }
    bool isGodMode() const { return m_godMode; }

private:
    void execCommand(const std::string& cmd, Game& game);

    // Input callback used by ImGui
    static int textEditCallback(struct ImGuiInputTextCallbackData* data);

    bool m_isOpen;
    bool m_godMode;

    // Input buffer for the text field
    char m_inputBuf[256];

    // Scrollable output log
    std::vector<std::string> m_log;
    std::string m_fullLog;
    // Command history (up/down arrow)
    std::vector<std::string> m_cmdHistory;
    int m_historyPos; // -1 = new line, 0..n = browsing history

    bool m_scrollToBottom;
    bool m_reclaimFocus;
};
