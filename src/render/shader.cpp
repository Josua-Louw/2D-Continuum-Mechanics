#include <render/shader.h>

#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>

namespace render {

Shader::~Shader() {
    // A program id of 0 means we never created (or already destroyed) it.
    if (m_id != 0) {
        glDeleteProgram(m_id);
    }
}

Shader::Shader(Shader&& other) noexcept : m_id(other.m_id) {
    other.m_id = 0;  // Steal the id so the moved-from object won't delete it.
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_id != 0) {
            glDeleteProgram(m_id);
        }
        m_id = other.m_id;
        other.m_id = 0;
    }
    return *this;
}

std::string Shader::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Shader: cannot open file: " << path << "\n";
        return {};
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

unsigned int Shader::compile(GLenum type, const std::string& source) {
    // A shader is created as empty, given its GLSL source, then compiled.
    // Compilation is a GPU-side operation that can fail and produce a log.
    unsigned int shader = glCreateShader(type);
    const char* text = source.c_str();
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);

    int ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == GL_TRUE) {
        return shader;
    }

    // On failure, pull the compiler log out of the driver and report it.
    char log[1024];
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    std::cerr << "Shader compile error (" << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << "): " << log << "\n";
    glDeleteShader(shader);
    return 0;
}

bool Shader::load(const std::string& vertexPath, const std::string& fragmentPath) {
    // Compile both stages, then attach them to a program and link it.
    // Attached shaders can be deleted once linked; the program keeps a copy.
    unsigned int vertex = compile(GL_VERTEX_SHADER, readFile(vertexPath));
    unsigned int fragment = compile(GL_FRAGMENT_SHADER, readFile(fragmentPath));
    if (vertex == 0 || fragment == 0) {
        return false;
    }

    m_id = glCreateProgram();
    glAttachShader(m_id, vertex);
    glAttachShader(m_id, fragment);
    glLinkProgram(m_id);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    int ok = GL_FALSE;
    glGetProgramiv(m_id, GL_LINK_STATUS, &ok);
    if (ok == GL_TRUE) {
        return true;
    }

    char log[1024];
    glGetProgramInfoLog(m_id, sizeof(log), nullptr, log);
    std::cerr << "Shader: program link failed: " << log << "\n";
    glDeleteProgram(m_id);
    m_id = 0;
    return false;
}

void Shader::use() const {
    glUseProgram(m_id);
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    // Uniforms are addressed by string name; the location is looked up once.
    // isTranspose=false because GLM stores columns in the layout OpenGL expects.
    glUniformMatrix4fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE, &value[0][0]);
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(m_id, name.c_str()), 1, &value[0]);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(m_id, name.c_str()), value);
}

}  // namespace render