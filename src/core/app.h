#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <optional>
#include <vector>
#include <filesystem>

#include <core/imgui_layer.h>
#include <core/project.h>
#include <render/camera.h>
#include <render/mesh.h>
#include <render/shader.h>

struct GLFWwindow;

namespace core {

// The GUI is a state machine: the app is always in exactly one screen, and
// the main loop dispatches to that screen's handler. Transitions happen by
// changing m_state inside a handler (e.g. on a button press) and take effect
// the next frame. See PLAN.md "State machine".
//
//     MainMenu --New Project--> ProjectCreate --Create & Run--> Simulation
//         ^                                                            |
//         |<--------------------------Exit to Menu---------------------|
//         |                       (also ESC from any screen)
//         +--Settings--------------------------------------------------+
enum class AppState {
    MainMenu,        // Entry point: browse saved projects, start a new one.
    ProjectCreate,   // Form that configures a new project (mesh + material).
    Simulation,      // The loaded project's scene, run and edited.
    Settings,        // Global application preferences.
};

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

    // Which screen the app is showing; see the AppState enum above.
    AppState m_state = AppState::MainMenu;

    // The previous frame's ESC state, so ESC is treated as a key press rather
    // than "held": pressing it once navigates, holding it does nothing extra.
    bool m_wasEscPressed = false;

    // The currently loaded project (none on the menu screen).
    std::optional<Project> m_currentProject;

    // Cached list of projects in the projects directory (newest first).
    std::vector<ProjectInfo> m_recentProjects;

    // Where projects are stored. Defaults to ./projects/ next to the binary.
    std::filesystem::path m_projectsDir;

    // True while the ImGui demo browser is shown (toggleable from Settings).
    bool m_showDemoWindow = false;

    bool init();
    void cleanup();

    // Build the demo triangle grid that will become the FEM mesh (M3).
    void buildGrid();

    // Handle keyboard/mouse input for this frame.
    void processInput();

    // Run the UI handler for the current state, then draw the scene. Called
    // once per frame between ImGui::NewFrame and ImGui::Render.
    void update();

    // Draw the scene. Non-simulation screens are pure ImGui (the grid is only
    // rendered while a simulation is on screen).
    void render();

    // One ImGui screen per AppState value. Each handler owns its windows for
    // that frame and may change m_state to navigate away.
    void updateMainMenu();
    void updateProjectCreate();
    void updateSimulation();
    void updateSettings();

    // Project I/O helpers.
    void scanRecentProjects();
    bool saveCurrentProject();
    std::optional<Project> loadProject(const std::filesystem::path& path);
    void initializeSimulationFromProject(const Project& project);

    // GLFW callbacks are free functions; they need access to the App, which
    // we recover from the user pointer stored on the window.
    static void onFramebufferResize(GLFWwindow* window, int width, int height);
    static void onScroll(GLFWwindow* window, double xOffset, double yOffset);
};

}  // namespace core
