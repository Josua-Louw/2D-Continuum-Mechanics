#include <core/imgui_layer.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace core {

ImGuiLayer::ImGuiLayer(GLFWwindow* window) : m_window(window) {}

ImGuiLayer::~ImGuiLayer() {
    shutdown();
}

bool ImGuiLayer::init() {
    if (m_initialized) {
        return true;
    }
    if (m_window == nullptr) {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Set the ImGui ini file to a dedicated config directory in the project
    // root (.continuum2d/imgui.ini) so the repo stays clean and the config
    // stays with the project.
    std::filesystem::path iniPath = std::filesystem::current_path() / ".continuum2d" / "imgui.ini";
    std::filesystem::create_directories(iniPath.parent_path());
    ImGui::GetIO().IniFilename = iniPath.c_str();

    // Install the GLFW platform backend (chaining any callbacks App installed
    // beforehand) and the OpenGL3 renderer backend. The renderer uses its own
    // built-in loader, independent of GLEW.
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        std::fprintf(stderr, "ImGuiLayer: failed to initialize OpenGL3 backend\n");
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    ImGui::StyleColorsDark();
    m_initialized = true;
    return true;
}

void ImGuiLayer::shutdown() {
    if (!m_initialized) {
        return;
    }
    // Backends must go before the ImGui context; both while GL is current.
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

void ImGuiLayer::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

}  // namespace core
