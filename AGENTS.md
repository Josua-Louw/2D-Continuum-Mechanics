# AGENTS.md

2D continuum mechanics simulation engine in C++20 with OpenGL, built to learn
the math of a continuum mechanics module.

## Commands

```sh
cmake -B build
cmake --build build
./build/continuum2d
```

- No test, lint, or CI infrastructure exists yet — verify by building and running.
- `cmake -B build` re-configure is only needed when `CMakeLists.txt` changes.
- Build with `-Wall -Wextra`; the project sets no `CMAKE_BUILD_TYPE` default in
  source (choose via `cmake -B build -DCMAKE_BUILD_TYPE=Debug`).

## Dependencies

Ubuntu 24.04 apt: `cmake build-essential libglfw3-dev libglew-dev libglm-dev libeigen3-dev`.
All found via `find_package` in the root `CMakeLists.txt`. If configure fails,
the likely cause is a missing dev package (e.g. GLM has no other install path).
OpenGL must be found explicitly (`find_package(OpenGL REQUIRED)` + link
`OpenGL::GL`); relying on GLEW alone fails at link time with "DSO missing from
command line".

## Architecture

- `src/main.cpp` is the only source file so far. Engine loop lives here until
  the core/ module is extracted.
- `sim/` (physics: deformation gradient, strain, stress, FEM) and `render/`
  (OpenGL: shaders, VAO/VBO) must NOT include each other. `sim` produces node
  positions + triangle topology; `render` only draws them.
- `math/` wraps GLM (GLSL-compatible). `shaders/` holds GLSL files.
- `src` is on the include path, so headers are included as `render/foo.h`,
  `sim/bar.h`, not relative paths.

## Current state

Milestone 1 only: GLFW window opens, clears to dark grey, `ESC` quits. No
shaders, meshes, or sim code yet. Empty dirs (`core/`, `math/`, `render/`,
`sim/`, `shaders/`) are intentional placeholders.
