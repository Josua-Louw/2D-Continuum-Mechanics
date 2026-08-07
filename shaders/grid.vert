#version 330 core

// Vertex shader for 2D meshes.
// The mesh provides 2D positions (location 0); the camera's combined
// view-projection matrix moves each vertex from world space into clip space.
layout(location = 0) in vec2 aPos;

uniform mat4 uViewProj;

void main() {
    gl_Position = uViewProj * vec4(aPos, 0.0, 1.0);
}
