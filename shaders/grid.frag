#version 330 core

// Fragment shader for 2D meshes: paints every fragment of the mesh with a
// single flat colour set from the CPU. Later milestones will replace this
// with per-vertex attributes (e.g. stress or strain heatmaps).
uniform vec4 uColor;

out vec4 FragColor;

void main() {
    FragColor = uColor;
}
