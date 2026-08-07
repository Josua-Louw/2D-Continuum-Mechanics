#include <render/camera.h>

#include <glm/gtc/matrix_transform.hpp>

namespace render {

void Camera::setViewport(float width, float height) {
    m_aspect = width / height;
}

glm::mat4 Camera::viewProjection() const {
    // Orthographic projection: the visible world rectangle is centred on
    // m_center and spans [-halfWidth, halfWidth] x [-scale, scale].
    const float halfWidth = m_scale * m_aspect;
    glm::mat4 projection = glm::ortho(-halfWidth, halfWidth, -m_scale, m_scale, -1.0f, 1.0f);

    // The view transform translates the world so the camera centre maps to
    // the origin, after which the projection places it at screen centre.
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(-m_center, 0.0f));

    return projection * view;
}

glm::vec2 Camera::screenToWorld(const glm::vec2& pixel, float width, float height) const {
    // Convert pixel (origin top-left, Y down) to NDC (origin centre, Y up).
    glm::vec2 ndc((2.0f * pixel.x) / width - 1.0f, 1.0f - (2.0f * pixel.y) / height);

    // NDC spans half the visible width/height in each direction around centre.
    return m_center + ndc * glm::vec2(m_scale * m_aspect, m_scale);
}

}  // namespace render