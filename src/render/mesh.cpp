#include <render/mesh.h>

namespace render {

Mesh::~Mesh() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
    }
    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
    }
}

Mesh::Mesh(Mesh&& other) noexcept
    : m_vao(other.m_vao),
      m_vbo(other.m_vbo),
      m_ebo(other.m_ebo),
      m_vertexCount(other.m_vertexCount),
      m_indexCount(other.m_indexCount) {
    // Leave the moved-from object owning nothing so its destructor is a no-op.
    other.m_vao = other.m_vbo = other.m_ebo = 0;
    other.m_vertexCount = other.m_indexCount = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        this->~Mesh();
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;
        m_vertexCount = other.m_vertexCount;
        m_indexCount = other.m_indexCount;
        other.m_vao = other.m_vbo = other.m_ebo = 0;
        other.m_vertexCount = other.m_indexCount = 0;
    }
    return *this;
}

void Mesh::ensureBuffers() {
    if (m_vao != 0) {
        return;
    }

    // One VAO records the state for all buffers involved in a draw call.
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // Attribute 0 = 2D position (matches layout(location = 0) in the shader).
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);

    // The element buffer holds triangle indices; GL knows the EBO binding is
    // part of the VAO, so it stays associated with it.
    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    glBindVertexArray(0);
}

void Mesh::setVertices(std::vector<glm::vec2> positions) {
    ensureBuffers();
    m_vertexCount = positions.size();
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertexCount * sizeof(glm::vec2), positions.data(), GL_STATIC_DRAW);
}

void Mesh::setTriangles(std::vector<unsigned int> indices) {
    ensureBuffers();
    m_indexCount = indices.size();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indexCount * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
}

void Mesh::draw() const {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, nullptr);
}

void Mesh::drawWireframe() const {
    // GL_POLYGON_MODE affects how subsequently-drawn polygons are rasterized;
    // switch back to filled after this so later draws are unaffected.
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, nullptr);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

}  // namespace render