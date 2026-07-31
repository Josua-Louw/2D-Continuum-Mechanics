# 2D Continuum Mechanics

A 2D simulation engine written in C++, built to implement the mathematics
taught in a continuum mechanics module. It uses OpenGL for rendering as a way
to learn graphics programming while visualizing the mechanics.

## Goals

The engine is being built to model and visualize the core objects of
continuum mechanics in 2D:

- **Deformation** — the deformation gradient `F` mapping reference to current
  configuration
- **Strain** — measures such as the Green-Lagrange strain `E`
- **Stress** — Cauchy stress `σ` and related measures
- **Constitutive laws** — e.g. neo-Hookean material response
- **Numerical solution** — finite element method (FEM) on a 2D triangle mesh,
  with sparse linear solves via Eigen

The intent is that each new concept from the module is implemented, rendered,
and explored as it is learned.

## Dependencies

All dependencies are installable from apt on Ubuntu 24.04:

```sh
sudo apt install cmake build-essential libglfw3-dev libglew-dev libglm-dev libeigen3-dev
```

| Library | Purpose |
| --- | --- |
| GLFW | Window creation, input, and OpenGL context |
| GLEW | OpenGL function loading |
| GLM | Header-only math (GLSL-compatible vectors/matrices) |
| Eigen | Linear algebra and sparse solvers for FEM |

## Build

```sh
cmake -B build
cmake --build build
./build/continuum2d
```

Press `ESC` or close the window to quit.

## Layout

```
src/
├── main.cpp    # entrypoint: engine loop
├── core/       # window, input, timestep
├── render/     # shaders, meshes (VAO/VBO), draw helpers
├── sim/        # continuum mechanics: F, strain, stress, FEM assembly
└── math/       # thin wrappers over GLM
shaders/        # GLSL vertex/fragment shaders
```

Design rule: `sim/` computes node positions and triangle topology; `render/`
only draws what it is given. The two layers never include each other, so
physics and graphics can be developed independently.

## Status

- [x] Toolchain scaffold: CMake project, GLFW window opening and clearing
- [ ] Shader pipeline and mesh rendering (triangle grid)
- [ ] Continuum mechanics core (`F`, strain, stress)
- [ ] FEM assembly and sparse solve
