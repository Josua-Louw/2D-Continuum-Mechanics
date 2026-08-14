#pragma once

#include <glm/glm.hpp>

#include <memory>

#include <core/imgui_layer.h>
#include <render/camera.h>
#include <render/mesh.h>
#include <render/shader.h>

struct GLFWwindow;

namespace core {

// Application shell: owns the GLFW window, the OpenGL scene (shader, demo
// grid, camera), and the main loop. main.cpp only constructs this and calls
// run(). Keeping the loop here (rather than in main) lets the engine be
// driven programmatically later, e.g. stepping the sim without a window.
class App {
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    // Create the window and scene, then run until the user quits.
    // Returns 0 on success, non-zero if initialisation failed.
    int run();

private:
    GLFWwindow* m_window = nullptr;
    int m_width = 1280;
    int m_height = 720;

    // Heap-owned because the GL context must be current when these are
    // destroyed; cleanup() resets them before the window is torn down.
    std::unique_ptr<render::Shader> m_shader;
    std::unique_ptr<render::Mesh> m_grid;
    render::Camera m_camera;
    std::unique_ptr<ImGuiLayer> m_imgui;

    // Mouse-pan bookkeeping: the last cursor position (in pixels) and whether
    // the left button is currently held.
    glm::dvec2 m_lastCursor{0.0, 0.0};
    bool m_dragging = false;

    bool init();
    void cleanup();

    // Build the demo triangle grid that will become the FEM mesh (M3).
    void buildGrid();

    // Handle keyboard/mouse input for this frame.
    void processInput();

    // Clear the screen and draw the grid (filled then wireframe).
    void render();

    // Phase-1 placeholder overlay: proves the ImGui context works until the
    // state machine and real screens land.
    void showImGuiDebugWindow();
    bool m_showDemoWindow = false;

    // GLFW callbacks are free functions; they need access to the App, which
    // we recover from the user pointer stored on the window.
    static void onFramebufferResize(GLFWwindow* window, int width, int height);
    static void onScroll(GLFWwindow* window, double xOffset, double yOffset);
};

}  // namespace core
