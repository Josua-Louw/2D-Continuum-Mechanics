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
- `render` (STATIC): OpenGL (shaders, meshes, camera). Links `math`, `glm::glm`.
- `continuum2d`: the app. Links `render` + `sim` + OpenGL/GLFW/GLEW.
- `sim_tests`: Catch2 tests. Links `sim` + `math` + `Catch2::Catch2WithMain`
  ONLY — headless, no OpenGL/GLFW, so tests run without a display.

## Architecture

- `src/main.cpp` is the only non-trivial source so far. Engine loop lives here
  until the core/ module is extracted (M2).
- `sim/` (physics: deformation gradient, strain, stress, FEM, contact) and
  `render/` (OpenGL: shaders, VAO/VBO) must NOT include each other. `sim`
  produces node positions + triangle topology; `render` only draws them.
- `math/` wraps GLM (GLSL-compatible). `shaders/` holds GLSL files.
- `src` is on the include path, so headers are included as `render/foo.h`,
  `sim/bar.h`, not relative paths.
- Sim uses a force-based residual formulation (`R = f_int - f_ext - f_c`) so
  dynamics can be added later; see `PLAN.md`.

## Current state

Milestone 1 only: GLFW window opens, clears to dark grey, `ESC` quits. Test
infrastructure is wired (CTest + one trivial Catch2 test). No shaders, meshes,
or sim code yet. `src/sim/sim.cpp` and `src/render/render.cpp` are zero-byte
placeholders required by CMake — replace them as modules land. Empty dirs
(`core/`, `math/`, `shaders/`) are intentional placeholders.
