#include <core/app.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <cstdio>

namespace core {

namespace {
constexpr const char* TITLE = "2D Continuum Mechanics";
constexpr const char* VERTEX_SHADER_PATH = "shaders/grid.vert";
constexpr const char* FRAGMENT_SHADER_PATH = "shaders/grid.frag";

// The demo grid: GRID_CELLS subdivisions per side, so (GRID_CELLS+1)^2 nodes
// and 2*GRID_CELLS^2 triangles. Extent chosen so the whole grid fits in the
// default camera view (scale = 1.0 -> half-height 1.0).
constexpr int GRID_CELLS = 24;
constexpr float GRID_HALF_EXTENT = 1.2f;

void glfwErrorCallback(int code, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", code, description);
}

// Flags shared by the full "screen" windows (menu, create, settings): no title
// bar, fixed position/size, nothing persisted to the ImGui .ini file (so every
// launch starts from a known layout), and no nav focus stealing.
constexpr ImGuiWindowFlags SCREEN_FLAGS =
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav;

// Begin a centred, auto-sized window intended to be a whole screen (menu,
// new-project form, settings). Caller must pair it with ImGui::End().
// The pivot (0.5, 0.5) in SetNextWindowPos centres the window on the display.
void beginCenteredWindow(const char* title) {
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin(title, nullptr, SCREEN_FLAGS);
}

// Standard header for the screen windows: app title + one-line description.
void screenHeader(const char* subtitle) {
    ImGui::TextUnformatted("2D Continuum Mechanics");
    ImGui::TextDisabled("%s", subtitle);
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 10.0f));
}
}  // namespace

App::App() = default;

App::~App() {
    // If run() already cleaned up, this is a no-op; otherwise it makes the
    // object safe to destroy without ever having run.
    cleanup();
}

bool App::init() {
    glfwSetErrorCallback(glfwErrorCallback);
    if (glfwInit() != GLFW_TRUE) {
        std::fprintf(stderr, "App: failed to initialize GLFW\n");
        return false;
    }

    // Request a modern core-profile context (GL 3.3); GLFW will pick a
    // compatible driver-provided context and create a window for it.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_width, m_height, TITLE, nullptr, nullptr);
    if (m_window == nullptr) {
        std::fprintf(stderr, "App: failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);  // V-sync: cap frames to the display refresh rate.

    // GLEW loads the OpenGL entry points for this context. glewExperimental is
    // required to expose core-profile functions. glewInit may raise a benign
    // GL_INVALID_ENUM which we clear.
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::fprintf(stderr, "App: failed to initialize GLEW\n");
        return false;
    }
    glGetError();

    m_shader = std::make_unique<render::Shader>();
    // Paths are relative to the working directory; run from the repo root.
    if (!m_shader->load(VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH)) {
        return false;
    }

    m_grid = std::make_unique<render::Mesh>();
    buildGrid();

    // Store a back-pointer so callbacks can reach this App instance.
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, &App::onFramebufferResize);
    glfwSetScrollCallback(m_window, &App::onScroll);

    // ImGui must initialise after the GL context is current (and after App's
    // callbacks are registered, so its own callbacks chain onto them).
    m_imgui = std::make_unique<ImGuiLayer>(m_window);
    if (!m_imgui->init()) {
        return false;
    }

    m_camera.setViewport(static_cast<float>(m_width), static_cast<float>(m_height));
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);

    return true;
}

void App::cleanup() {
    // Order matters: the mesh, shader and ImGui backends own GL resources, so
    // they must be released while the context is still current, before the
    // window goes.
    if (m_imgui != nullptr) {
        m_imgui.reset();
    }
    if (m_shader != nullptr) {
        m_shader.reset();
    }
    if (m_grid != nullptr) {
        m_grid.reset();
    }
    if (m_window != nullptr) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        glfwTerminate();
    }
}

void App::buildGrid() {
    // A regular grid of cells, each split into two triangles. Two arrays are
    // produced: one of node positions, one of triangle vertex indices.
    // Vertex (i, j) lives at index j * (GRID_CELLS + 1) + i.
    std::vector<glm::vec2> positions;
    constexpr float step = 2.0f * GRID_HALF_EXTENT / GRID_CELLS;
    positions.reserve((GRID_CELLS + 1) * (GRID_CELLS + 1));
    for (int j = 0; j <= GRID_CELLS; ++j) {
        for (int i = 0; i <= GRID_CELLS; ++i) {
            positions.emplace_back(-GRID_HALF_EXTENT + i * step, -GRID_HALF_EXTENT + j * step);
        }
    }

    std::vector<unsigned int> indices;
    indices.reserve(2 * GRID_CELLS * GRID_CELLS * 3);
    for (int j = 0; j < GRID_CELLS; ++j) {
        for (int i = 0; i < GRID_CELLS; ++i) {
            // Node ids of the cell corners (bottom-left, bottom-right,
            // top-right, top-left) and the two triangles that fill the cell.
            const unsigned int bl = static_cast<unsigned int>(j * (GRID_CELLS + 1) + i);
            const unsigned int br = bl + 1;
            const unsigned int tl = static_cast<unsigned int>((j + 1) * (GRID_CELLS + 1) + i);
            const unsigned int tr = tl + 1;
            indices.insert(indices.end(), {bl, br, tl, br, tr, tl});
        }
    }

    m_grid->setVertices(std::move(positions));
    m_grid->setTriangles(std::move(indices));
}

void App::processInput() {
    // --- ESC navigation -----------------------------------------------------
    // ESC acts as "back": it quits from the main menu and returns to the menu
    // from anywhere else. It is edge-triggered (fires on the press, not while
    // held) so holding ESC never navigates twice. When ImGui is editing text
    // or has a popup open, ESC belongs to ImGui instead (clear field / close
    // popup) and we leave it alone.
    const bool escPressed = glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    const bool imguiWantsEsc =
        ImGui::IsPopupOpen(nullptr,
                           ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel) ||
        ImGui::GetIO().WantTextInput;
    if (escPressed && !m_wasEscPressed && !imguiWantsEsc) {
        if (m_state == AppState::MainMenu) {
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        } else {
            m_state = AppState::MainMenu;
        }
    }
    m_wasEscPressed = escPressed;

    // --- Mouse panning -----------------------------------------------------
    // Panning only makes sense while the scene is visible (Simulation state),
    // and ImGui wants the mouse whenever it hovers or drags a widget, so hand
    // it over in those cases too.
    if (m_state != AppState::Simulation ||
        (m_imgui != nullptr && ImGui::GetIO().WantCaptureMouse)) {
        m_dragging = false;
        return;
    }

    // Left-drag pans the view: the world points under the cursor before and
    // after the mouse moves must stay aligned, so we shift the camera centre
    // by the cursor's world-space displacement.
    const bool pressed = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(m_window, &x, &y);
    const glm::vec2 cursor(static_cast<float>(x), static_cast<float>(y));

    if (pressed) {
        if (m_dragging) {
            const glm::vec2 currentWorld = m_camera.screenToWorld(cursor, m_width, m_height);
            const glm::vec2 lastWorld = m_camera.screenToWorld(m_lastCursor, m_width, m_height);
            // Shift the view by the cursor's world-space displacement so the
            // content under the cursor stays under it (moves with the mouse).
            m_camera.pan(lastWorld - currentWorld);
        } else {
            // First frame of the drag: just remember where it started.
            m_dragging = true;
        }
    } else {
        m_dragging = false;
    }
    m_lastCursor = glm::dvec2(cursor);
}

void App::update() {
    // Dispatch to the handler for the current screen. A handler may change
    // m_state (navigation); the new screen takes over from the next frame.
    switch (m_state) {
        case AppState::MainMenu:      updateMainMenu();      break;
        case AppState::ProjectCreate: updateProjectCreate(); break;
        case AppState::Simulation:    updateSimulation();    break;
        case AppState::Settings:      updateSettings();      break;
    }
}

void App::render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Only the simulation screen shows the scene. The menu, new-project and
    // settings screens are pure ImGui, so the plain background is all there is
    // to draw behind them.
    if (m_state != AppState::Simulation) {
        return;
    }

    // Draw the grid twice: a dim filled pass underneath, then a brighter
    // wireframe on top so cell structure stays visible. Both use the same
    // mesh and shader; only the colour and polygon mode change.
    m_shader->use();
    m_shader->setMat4("uViewProj", m_camera.viewProjection());

    m_shader->setVec4("uColor", glm::vec4(0.30f, 0.31f, 0.36f, 1.0f));
    m_grid->draw();

    m_shader->setVec4("uColor", glm::vec4(0.55f, 0.57f, 0.65f, 1.0f));
    m_grid->drawWireframe();
}

void App::updateMainMenu() {
    // The main menu is a compact, centred window on a plain background. It is
    // the app's entry point: start or load a project, tweak settings, quit.
    beginCenteredWindow("Main Menu");

    screenHeader("Phase 2: state-machine skeleton");

    const ImVec2 buttonSize(240.0f, 40.0f);

    // Start a fresh project: drop into the creation screen (the real form is
    // built in Phase 5).
    if (ImGui::Button("New Project", buttonSize)) {
        m_state = AppState::ProjectCreate;
    }

    // Browsing/opening saved projects needs the project model (Phase 3) and
    // the browser UI (Phase 4), so the entry point stays visible but inert.
    ImGui::BeginDisabled();
    ImGui::Button("Load Project", buttonSize);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Saved projects will be listed here (Phase 4).");
    }

    if (ImGui::Button("Settings", buttonSize)) {
        m_state = AppState::Settings;
    }

    if (ImGui::Button("Quit", buttonSize)) {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }

    ImGui::End();
}

void App::updateProjectCreate() {
    beginCenteredWindow("New Project");

    screenHeader("Phase 2: skeleton - form fields arrive in Phase 5.");

    // Placeholder outline of the fields the creation form will edit once the
    // Project model (Phase 3) exists. Showing them keeps the layout obvious.
    ImGui::BulletText("Project name");
    ImGui::BulletText("Mesh: cells per side, world size");
    ImGui::BulletText("Material: shear / bulk modulus, density");

    ImGui::Dummy(ImVec2(0.0f, 12.0f));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    const ImVec2 buttonSize(140.0f, 36.0f);
    if (ImGui::Button("Cancel", buttonSize)) {
        m_state = AppState::MainMenu;
    }
    ImGui::SameLine();
    if (ImGui::Button("Create & Run", buttonSize)) {
        // Phase 5: validate the form, build the sim mesh, then enter the
        // simulation screen. For now it jumps straight to the simulation view.
        m_state = AppState::Simulation;
    }

    ImGui::End();
}

void App::updateSimulation() {
    // The scene fills the window behind ImGui. This small overlay reports the
    // state while the full project view (menus, tools, save/load) is designed
    // together with the sim module.
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320.0f, 140.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Simulation");
    ImGui::TextDisabled("Phase 2: project view is a stub.");
    ImGui::Text("Left-drag pans, scroll zooms, ESC returns to menu.");
    ImGui::Separator();
    ImGui::Text("ImGui v%s", IMGUI_VERSION);
    ImGui::Text("FPS: %.1f (%.2f ms)", ImGui::GetIO().Framerate,
                1000.0f / ImGui::GetIO().Framerate);
    ImGui::End();
}

void App::updateSettings() {
    beginCenteredWindow("Settings");

    screenHeader("Phase 2: skeleton - preferences land in Phase 7.");

    // Handy while developing: toggle the full ImGui demo browser, which shows
    // every widget and lets us test interactions without writing a UI first.
    ImGui::Checkbox("Show ImGui demo window", &m_showDemoWindow);
    if (m_showDemoWindow) {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }

    ImGui::Dummy(ImVec2(0.0f, 12.0f));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    const ImVec2 buttonSize(140.0f, 36.0f);
    if (ImGui::Button("Back", buttonSize)) {
        m_state = AppState::MainMenu;
    }

    ImGui::End();
}

void App::onFramebufferResize(GLFWwindow* window, int width, int height) {
    // Called by GLFW whenever the window is resized; keep the GL viewport and
    // the camera's aspect ratio in sync with the new framebuffer size.
    auto* self = static_cast<App*>(glfwGetWindowUserPointer(window));
    self->m_width = width;
    self->m_height = height;
    glViewport(0, 0, width, height);
    self->m_camera.setViewport(static_cast<float>(width), static_cast<float>(height));
}

void App::onScroll(GLFWwindow* window, double /*xOffset*/, double yOffset) {
    auto* self = static_cast<App*>(glfwGetWindowUserPointer(window));
    // Exponential zoom so each scroll notch changes the scale by a constant
    // ratio (feels smooth regardless of current zoom). Positive scroll (up)
    // zooms in.
    self->m_camera.zoom(static_cast<float>(std::exp(-yOffset * 0.15)));
}

int App::run() {
    if (!init()) {
        cleanup();
        return 1;
    }

    while (glfwWindowShouldClose(m_window) == GLFW_FALSE) {
        m_imgui->beginFrame();
        processInput();
        update();
        render();
        m_imgui->endFrame();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

    cleanup();
    return 0;
}

}  // namespace core