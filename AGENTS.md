# AGENTS.md

2D continuum mechanics simulation engine in C++20 with OpenGL, built to learn
the math of a continuum mechanics module.

## Commands

```sh
cmake -B build
cmake --build build
./build/continuum2d
ctest --test-dir build --output-on-failure
```

- No lint or CI infrastructure exists yet — verify by building, running, and
  `ctest`. See `PLAN.md` for the roadmap and testing strategy.
- `cmake -B build` re-configure is only needed when `CMakeLists.txt` changes.
- Build with `-Wall -Wextra`; the project sets no `CMAKE_BUILD_TYPE` default in
  source (choose via `cmake -B build -DCMAKE_BUILD_TYPE=Debug`).
- Tests can be disabled with `-DBUILD_TESTING=OFF`.

## Dependencies

Ubuntu 24.04 apt: `cmake build-essential libglfw3-dev libglew-dev libglm-dev libeigen3-dev catch2`.
All found via `find_package` in the root `CMakeLists.txt`. If configure fails,
the likely cause is a missing dev package (e.g. GLM has no other install path).
OpenGL must be found explicitly (`find_package(OpenGL REQUIRED)` + link
`OpenGL::GL`); relying on GLEW alone fails at link time with "DSO missing from
command line".

## Targets

- `math` (INTERFACE): header-only GLM wrappers; carries the `src` include path.
- `sim` (STATIC): continuum mechanics (mesh, F, strain, stress, FEM, contact).
  Links `math`, `Eigen3::Eigen`.
- `render` (STATIC): OpenGL (shaders, meshes, camera). Links `math`, `glm::glm`,
  `GLEW::GLEW`.
- `core` (STATIC): window, input, main loop. Links `render` + OpenGL/GLFW/GLEW;
  owns the GL context.
- `continuum2d`: the app. Links `core`.
- `sim_tests`: Catch2 tests. Links `sim` + `math` + `Catch2::Catch2WithMain`
  ONLY — headless, no OpenGL/GLFW, so tests run without a display.

## Architecture

- `src/main.cpp` is a thin entrypoint; the loop lives in `core/app.{h,cpp}`
  (owns the window + GL context, drives render + sim).
- `sim/` (physics: mesh, deformation gradient, strain, stress, FEM, contact)
  and `render/` (OpenGL: shaders, VAO/VBO, camera) must NOT include each
  other. `sim` produces node positions + triangle topology; `render` only
  draws them.
- `math/` wraps GLM (GLSL-compatible). `shaders/` holds GLSL files.
- `src` is on the include path, so headers are included as `render/foo.h`,
  `sim/bar.h`, not relative paths.
- Sim uses a force-based residual formulation (`R = f_int - f_ext - f_c`) so
  dynamics can be added later; see `PLAN.md`.
- GL objects (Shader/Mesh) are destroyed while the GL context is current
  (`core::App::cleanup` resets them before the window is torn down).
- Shader paths in `core/app.cpp` are relative to the working directory — run
  the binary from the repo root.

## Current state

Milestone 2: GLFW window with a 24×24 triangle grid drawn filled + wireframe;
left-drag pans, scroll zooms, `ESC` quits. Test infrastructure wired (CTest +
Catch2). No sim code yet — `src/sim/sim.cpp` is a zero-byte placeholder
required by CMake; replace it as the module lands. The demo grid is built in
`core/app.cpp`; M3 will feed `render::Mesh` from `sim::Mesh`. Empty dirs
(`math/`) are intentional placeholders.
