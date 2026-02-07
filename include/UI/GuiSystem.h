#pragma once
#include <memory>
#include "imgui.h"

struct GLFWwindow;

class GuiSystem {
public:
    GuiSystem(GLFWwindow* window);
    ~GuiSystem();

    void beginFrame();
    void endFrame();
    void render();

    ImFont* getFont() const { return m_font; }
    ImFont* getBigFont() const { return m_bigFont; }

private:
    GLFWwindow* m_window;
    ImFont* m_font;
    ImFont* m_bigFont;
};
