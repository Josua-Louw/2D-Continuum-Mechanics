#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <string>

namespace render {

// Wraps an OpenGL shader program: vertex + fragment shaders compiled from
// source files on disk, linked into one program that can be bound and given
// uniforms. Errors are reported to stderr; a failed load leaves the program
// id at 0, which the caller can treat as "no shader".
//
// Ownership note: the program lives on the GPU and is only valid while an
// OpenGL context exists, so destroy the Shader while the context is current.
class Shader {
public:
    Shader() = default;
    ~Shader();

    // Non-copyable: two copies of the same id would double-delete the program.
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    // Compile and link the vertex/fragment shaders stored at these paths.
    // Paths are relative to the process working directory.
    bool load(const std::string& vertexPath, const std::string& fragmentPath);

    void use() const;

    void setMat4(const std::string& name, const glm::mat4& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setFloat(const std::string& name, float value) const;

private:
    unsigned int m_id = 0;

    // Compile one shader stage from GLSL source; returns 0 on failure.
    static unsigned int compile(GLenum type, const std::string& source);
    static std::string readFile(const std::string& path);
};

}  // namespace render
