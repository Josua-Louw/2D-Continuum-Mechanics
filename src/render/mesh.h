#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <vector>

namespace render {

// A GPU mesh: a list of 2D node positions plus a list of triangle indices
// (each group of three indices names the corners of one triangle). The
// positions are uploaded to a vertex buffer and the indices to an element
// buffer; both live behind a single vertex array object (VAO) that records
// how the position attribute is laid out.
//
// This mirrors how the sim layer will hand data to rendering: sim produces
// node positions + triangle topology, render uploads them and draws.
//
// Ownership note: like Shader, the GL buffers require a current context, so
// destroy the Mesh while the context is still current.
class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // Upload new geometry. Each call replaces the previous data.
    void setVertices(std::vector<glm::vec2> positions);
    void setTriangles(std::vector<unsigned int> indices);

    // Draw the triangles; wireframe mode renders only their edges.
    void draw() const;
    void drawWireframe() const;

    std::size_t triangleCount() const { return m_indexCount / 3; }

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    std::size_t m_vertexCount = 0;
    std::size_t m_indexCount = 0;

    // Create buffers and configure the attribute layout if they don't exist
    // yet. Called lazily so an empty Mesh never touches the GPU.
    void ensureBuffers();
};

}  // namespace render