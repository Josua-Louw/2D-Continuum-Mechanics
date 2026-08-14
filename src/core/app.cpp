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
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }

    // Left-drag pans the view: the world points under the cursor before and
    // after the mouse moves must stay aligned, so we shift the camera centre
    // by the cursor's world-space displacement.
    const bool pressed = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(m_window, &x, &y);
    const glm::vec2 cursor(static_cast<float>(x), static_cast<float>(y));

    // When ImGui wants the mouse (hovering/dragging a widget), hand over the
    // panning so interacting with the UI doesn't also shift the camera.
    if (m_imgui != nullptr && ImGui::GetIO().WantCaptureMouse) {
        m_dragging = false;
        m_lastCursor = glm::dvec2(cursor);
        return;
    }

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

void App::render() {
    glClear(GL_COLOR_BUFFER_BIT);

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

void App::showImGuiDebugWindow() {
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320.0f, 150.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Continuum2D", nullptr)) {
        ImGui::Text("ImGui v%s", IMGUI_VERSION);
        ImGui::Text("Frame rate: %.1f FPS (%.3f ms)", ImGui::GetIO().Framerate,
                    1000.0f / ImGui::GetIO().Framerate);
        ImGui::Checkbox("Show ImGui demo window", &m_showDemoWindow);
    }
    ImGui::End();

    if (m_showDemoWindow) {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }
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
        render();
        showImGuiDebugWindow();
        m_imgui->endFrame();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

    cleanup();
    return 0;
}

}  // namespace core