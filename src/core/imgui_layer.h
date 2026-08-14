#pragma once

struct GLFWwindow;

namespace core {

// Owns the Dear ImGui context and its GLFW/OpenGL3 backends. Constructed with
// the GLFW window and an already-current GL context; call beginFrame() at the
// start of each frame and endFrame() at the end (which renders the GUI on top
// of the scene).
//
// Ownership note: the backends reference the window and GL state, so destroy
// the ImGuiLayer (shutdown) while the context is still current, before the
// window is torn down — same rule as the other GL objects in App.
class ImGuiLayer {
public:
    explicit ImGuiLayer(GLFWwindow* window);
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    // Create the ImGui context and set up the GLFW + OpenGL3 backends. The
    // GLFW backend installs its own callbacks and chains any already installed
    // on the window, so App must register its callbacks before calling init().
    bool init();
    void shutdown();

    // Call once per frame, in order, from the main loop.
    void beginFrame();
    void endFrame();

private:
    GLFWwindow* m_window = nullptr;
    bool m_initialized = false;
};

}  // namespace core
