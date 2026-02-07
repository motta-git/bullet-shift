#include "GuiSystem.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Core/Config.h"

#include <GLFW/glfw3.h>

GuiSystem::GuiSystem(GLFWwindow* window) : m_window(window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr; // Disable ini saving/loading for better stability in Wine
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows (Disabled for Wine stability)

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    // Load custom font
    const char* fontPath = Config::FONT_PATH;
    m_font = io.Fonts->AddFontFromFileTTF(fontPath, 18.0f);
    m_bigFont = io.Fonts->AddFontFromFileTTF(fontPath, 48.0f);

    if (!m_font) {
        m_font = io.Fonts->AddFontDefault();
    }
    if (!m_bigFont) {
        m_bigFont = m_font;
    }
}

GuiSystem::~GuiSystem() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void GuiSystem::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GuiSystem::endFrame() {
    ImGui::Render();
}

void GuiSystem::render() {
    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData == nullptr) {
        return;
    }
    ImGui_ImplOpenGL3_RenderDrawData(drawData);

    /* Viewports disabled for Wine compatibility
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
    */
}
