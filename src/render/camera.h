#pragma once

#include <glm/glm.hpp>

namespace render {

// A 2D orthographic camera. It maps a region of the world onto the window:
//   - m_center is the world point shown at the centre of the screen;
//   - m_scale (halfHeight) is half the world height visible on screen, so a
//     larger value zooms out; the visible width follows from the aspect ratio.
//
// World Y points up (matching the gravity convention used by sim), while
// screen pixels have Y pointing down, so screenToWorld flips Y.
class Camera {
public:
    // Aspect ratio is derived from the pixel size of the framebuffer, so it
    // must be refreshed whenever the window is resized.
    void setViewport(float width, float height);

    void setCenter(glm::vec2 center) { m_center = center; }
    glm::vec2 center() const { return m_center; }

    // Move the view by a world-space offset.
    void pan(glm::vec2 worldDelta) { m_center += worldDelta; }

    // Multiply the zoom level; factor > 1 zooms in, factor < 1 zooms out.
    void zoom(float factor) { m_scale = glm::clamp(m_scale * factor, 1e-4f, 1e4f); }

    // The combined model->NDC transform for the current view.
    glm::mat4 viewProjection() const;

    // Convert a pixel coordinate (origin top-left, Y down) to a world point.
    glm::vec2 screenToWorld(const glm::vec2& pixel, float width, float height) const;

private:
    glm::vec2 m_center{0.0f, 0.0f};
    float m_scale = 1.0f;
    float m_aspect = 1.0f;
};

}  // namespace render