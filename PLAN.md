# Plan

Reference document for the 2D Continuum Mechanics engine. Read this alongside
`AGENTS.md` (build/test commands and repo conventions) and `README.md` (project
description). This file tracks the roadmap, decisions, and testing strategy so
both you and the tooling stay oriented as work progresses.

## Goal

Implement the mathematics of a continuum mechanics module in a 2D C++/OpenGL
simulation engine. The current concrete target: compute **pressure** (normal
traction) and **shear** (friction traction) for an object in contact with a
rigid surface.

## Locked decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Simulation type | Quasi-static nonlinear FEM (Newton solve) | Simplest route to contact forces; matches typical module setup |
| Future dynamics | Design stays dynamic-ready | Residual/force-based formulation so time integration slots in later |
| Material model | Neo-Hookean (hyperelastic) | Compressible, rubber-like, valid at the large deformations contact produces |
| Contact target | Object vs. rigid floor | Standard first contact problem; no master/slave scheme needed |
| Test framework | Catch2 v3 (`apt catch2`) | Chosen over doctest/GoogleTest; links `Catch2::Catch2WithMain` |

## Architecture rules (non-negotiable)

1. `render/` never includes `sim/`. `sim` produces node positions + triangle
   topology + per-element scalars; `render` only draws them.
2. Force-based residual formulation: internal force `f_int = ∫ Bᵀ σ dΩ` is
   computed from stress directly. Static equilibrium is `R = f_int − f_ext −
   f_c = 0`. Later, dynamics reuses everything by adding a mass matrix:
   `R = M·a + f_int − f_ext − f_c`.
3. Contact forces enter the residual (`f_c`), not as a constraint hack — stays
   valid under time-stepping.
4. Tests are headless: test binaries link only `sim` + `math` (+ Catch2), never
   `render`/GLFW/GLEW. Enabled by rules 1–2.
5. `src` is on the include path; headers include as `render/foo.h`, `sim/bar.h`.

## Roadmap

### M1 — Toolchain scaffold (DONE)
GLFW window opens, clears, `ESC` quits. CMake project with
`glfw glew glm eigen OpenGL`.

### M2 — Render pipeline + test infrastructure (DONE)
- [x] `render/shader.{h,cpp}`: GLSL compile/link helper, loads from `shaders/`
- [x] `render/mesh.{h,cpp}`: VAO/VBO wrapper over positions + triangle indices
  (attribute 0 = `vec2` position; filled + wireframe draws)
- [x] `render/camera.{h,cpp}`: 2D orthographic pan/zoom; `screenToWorld` maps
  pixels → world (Y-up world, Y-down pixels)
- [x] Extract window/loop from `main.cpp` into `core/app.{h,cpp}`
  (`main.cpp` is now a thin entrypoint; `core` links GLFW/GLEW/OpenGL and owns
  the GL context)
- [x] Draw a triangle grid (24×24 cells) filled + wireframe; left-drag pans,
  scroll zooms, `ESC` quits
- [x] Test scaffold: CMake library split + CTest + trivial Catch2 test
- Shaders live in `shaders/grid.{vert,frag}`: `uViewProj` + flat `uColor`
  (M4 will swap flat colour for per-vertex stress/colour maps)
- GL objects are destroyed while the context is current (`core::App::cleanup`)
- Demo grid is built in `core/app.cpp`; M3 replaces it with `sim::Mesh` output

### M3 — Kinematics (`sim/`)
- `sim/mesh.{h,cpp}`: nodes + triangle elements, subdivided rectangle generator
- `sim/deformation.{h,cpp}`: per-element deformation gradient `F = ∂x/∂X`
- Hand-imposed displacement; color triangles by `F` invariant
- Tests: `F`, `E`, invariants, `J = det F` vs hand-computed fields; `F = I` ⇒
  stress-free

### M4 — Constitutive (`sim/material.{h,cpp}`)
- Green-Lagrange strain `E = ½(FᵀF − I)`
- Neo-Hookean 2nd Piola-Kirchhoff `S(E)`, Cauchy `σ = (1/J) F S Fᵀ`
- Stress heatmap on a stretched block
- Tests: σ vs closed-form biaxial stretch and pure shear; σ symmetry; zero
  residual at reference config

### M5 — Nonlinear FEM solve (`sim/fem.{h,cpp}`, `sim/solver.{h,cpp}`)
- Residual `R(u) = f_int(u) − f_ext`; Newton with `K = ∂f_int/∂u`
- Eigen `SimplicialLDLT` sparse solve
- Dirichlet (fixed nodes) + Neumann (surface traction) BCs; gravity body force
- Tests: K symmetry; **patch test** (constant strain reproduced exactly by every
  triangle); known small systems solve exactly; Newton convergence

### M6 — Contact: pressure & shear (`sim/contact.{h,cpp}`)
- Penalty normal contact vs. rigid floor (Signorini: gap ≥ 0, p ≥ 0, gap·p = 0)
- Coulomb friction `|t_t| ≤ μp`
- Traction recovery via Cauchy's theorem `t = σ·n` on contact edges → pressure
  `p = −nᵀσn`, shear `= |t − pn|`; integrate for total forces; render patch +
  force arrows
- Tests: gap ⇒ no force; penetration ⇒ positive pressure; **block on floor ⇒
  total normal force = weight**; friction bound `|t_t| ≤ μp`; traction
  recovery consistent with direct contact forces

## Testing strategy

Verification against closed-form math is the core idea — each feature ships with
tests proving the numerics match theory.

- **Framework**: Catch2 v3. Each test executable is registered with CTest.
- **Verification tiers**:
  1. Kinematics: hand-computed `F`, `E`, invariants, `J`
  2. Constitutive: closed-form neo-Hookean stress, symmetry, stress-free
     reference
  3. Assembly: stiffness symmetry, patch test
  4. Solver: analytic small systems, Newton convergence
  5. Contact: Signorini conditions, weight balance, friction bound, traction
     consistency
  6. Global: traction BC `σ·n = t`, strain energy = external work
- **Tolerances**: relative tolerance for solve/contact quantities; exact
  equality only for topology/indices.
- **Process**: write tests alongside each milestone's feature code, not at the
  end.

## Commands

```sh
# configure + build
cmake -B build && cmake --build build

# run app
./build/continuum2d

# run tests
ctest --test-dir build --output-on-failure
```

## Status

- [x] M1 toolchain scaffold
- [x] M2 render pipeline + test infrastructure
- [ ] M3 kinematics
- [ ] M4 constitutive
- [ ] M5 nonlinear FEM solve
- [ ] M6 contact (pressure & shear)

---

# GUI & System Controls (owner: GUI agent)

> This section is maintained by the agent responsible for the menu / project
> creation / save-load GUI. It lives in its own section so it does not clash
> with the physics agent's roadmap above. The two roadmap areas cooperate: the
> GUI drives the `sim` module via serialised `Project` data, while `sim`
> produces node positions + topology that the GUI's project view renders.

## Scope

A presentation layer on top of the engine that:

- **Menu page**: browse and open past projects, create a new project, reach
  settings.
- **Project creation page**: configure everything a simulation needs (mesh +
  material to begin, extended as sim milestones land).
- **Project view**: runs the simulation; must support **save** and **load**.
  The project view's layout/controls will be developed as the sim system is
  built, guided by the user.

## Locked decisions

| Decision | Choice | Why |
| --- | --- | --- |
| GUI library | Dear ImGui (GLFW + OpenGL3 backend) | Standard for OpenGL tools, immediate mode, easy to iterate |
| Project data scope | Minimal: mesh config + material config for now | Expand after M3–M6 add BCs / solver / contact fields |
| App integration | State machine, replaces flat render loop | Clean separation of Menu / Create / Simulation / Settings |
| Serialization | nlohmann/json (header-only) | Human-readable, versionable, easy migration |
| Save file format | `.json` per project | Keeps projects inspectable and future-proof |

## Architecture rules

- The GUI lives in `core/` (window + GL context owner). It never reaches into
  `sim/` internals; it hands `sim` serialised project data and renders what
  `sim` produces. The existing rule "`render/` never includes `sim/`" still
  holds — GUI state and drawing sit one layer above both.
- Project data (`core/project.h`) is pure data with zero `sim`/`render`
  dependencies, kept serialisable so it can grow independently of physics.

## State machine

```
MainMenu ──New Project──▶ ProjectCreate ──Create & Run──▶ Simulation
    ▲                                                            │
    │──────────────Load Project────────────────────────────────┘
    │                                                           │
    └──Settings──MainMenu  ◀────Exit to Menu────────────────────┘
```

`core::App` owns the current `AppState` and dispatches to a handler per state
in the main loop. `ESC` from Simulation returns to the menu.

## Project data model (`src/core/project.h/cpp`)

```cpp
struct MeshConfig {
    int gridCells = 24;    // subdivisions per side
    float halfExtent = 1.2f;   // world half-size
};

struct MaterialConfig {
    float shearModulus = 1000.0f;   // μ, Neo-Hookean
    float bulkModulus  = 2000.0f;   // λ, compressible Neo-Hookean
    float density      = 1000.0f;   // ρ, for future dynamics
};

struct Project {
    std::string name;
    std::string createdAt;
    std::string modifiedAt;
    MeshConfig mesh;
    MaterialConfig material;
    // future: boundary conditions, solver params, contact settings
    nlohmann::json toJson() const;
    static Project fromJson(const nlohmann::json&);
};

struct ProjectInfo {          // for the menu's project browser
    std::string name;
    std::filesystem::path path;
    std::string lastModified;
};
```

## ImGui integration (`src/core/imgui_layer.h/cpp`)

```cpp
class ImGuiLayer {
public:
    ImGuiLayer(GLFWwindow* window);
    ~ImGuiLayer();
    void beginFrame();  // NewFrame() for both backends
    void endFrame();    // ImGui::Render + RenderDrawData
};
```

Initialised in `App::init()` (context + GLFW/OpenGL3 backends, dark style).
The main loop becomes:

```
processInput();          // ESC / state transitions
updateCurrentState();    // dispatch to handler
render();                // existing simulation draw
ImGui::Render();         // GUI on top
glfwSwapBuffers; glfwPollEvents;
```

## File structure

```
src/
├── core/
│   ├── app.{h,cpp}           # state machine, ImGui layer, project I/O
│   ├── project.{h,cpp}       # project data model + JSON save/load
│   ├── imgui_layer.{h,cpp}   # ImGui init / render wrapper
│   └── (optional) file_dialog → ImGuiFileDialog (header-only)
├── render/                   # unchanged
├── sim/                      # unchanged (driven by project data)
└── main.cpp                  # unchanged
```

## Implementation phases

| Phase | Tasks | Deliverable | Status |
| --- | --- | --- | --- |
| 1. ImGui setup | Add deps, `ImGuiLayer`, wire into `App::init`/`run` | Working ImGui context | DONE |
| 2. State machine | `AppState` enum, dispatch in `run`, stub handlers | Clean transitions | DONE |
| 3. Project model | `project.{h,cpp}`, JSON save/load, scan projects | Persistence works | DONE |
| 4. Main menu | Project browser, New Project, Settings buttons | Usable menu | DONE |
| 5. Project create | Mesh/material form, validate, Create & Run | New projects configured | pending |
| 6. Simulation view | Menu bar, side panel, save/exit | Save + load working | pending |
| 7. Settings | Directory, rendering prefs, persistence | App preferences | pending |
| 8. Polish | Theme, shortcuts, error handling | Production-ready | pending |

## GUI progress log

What has actually landed, in implementation order. The physics roadmap above is
untouched; this tracks only the GUI section's own work.

### Phase 1 — ImGui setup (DONE)

- **Dependencies**: `nlohmann-json3-dev` added to the apt list in `AGENTS.md`;
  Dear ImGui is *not* an apt dependency. Ubuntu's `libimgui-dev` compiled
  ImGui 1.90.1 fine, but its GLFW/OpenGL3 backend `.cpp` files live under
  `/usr/share/doc/` and it needs sudo — awkward and fragile. Instead **ImGui is
  fetched at configure time via `FetchContent`** (v1.92.9, shallow clone). The
  upstream repo ships no root `CMakeLists.txt`, so `CMakeLists.txt` builds an
  `imgui` static target from the core files + GLFW/OpenGL3 backends itself.
  Needs network on the first `cmake -B build` (already noted in `AGENTS.md`).
- **`src/core/imgui_layer.{h,cpp}`** (new): owns the ImGui context, initialises
  the GLFW + OpenGL3 backends (`ImGui_ImplGlfw_InitForOpenGL` +
  `ImGui_ImplOpenGL3_Init("#version 330")`), and exposes `beginFrame()` /
  `endFrame()` around each rendered frame. Shutdown runs while the GL context
  is still current (same rule as the other GL objects in `App`).
- **`src/core/app.{h,cpp}`**: the layer is created in `App::init()` *after* the
  GL context is current and *after* App's GLFW callbacks, so ImGui chains onto
  them. Mouse panning now yields to `ImGui::GetIO().WantCaptureMouse`. The
  OpenGL3 backend uses its own bundled loader, so no GLEW interaction.
- Verified: builds clean with `-Wall -Wextra`, `ctest` passes, app runs.

### Phase 2 — State machine (DONE)

- **`enum class AppState`** in `app.h`: `MainMenu`, `ProjectCreate`,
  `Simulation`, `Settings`, with the transition diagram documented above it.
- **Dispatch**: `App::update()` runs the handler for the current state each
  frame; handlers set `m_state` to navigate, taking effect from the next frame.
  The `run()` loop is now `processInput(); update(); render();` inside the
  ImGui frame.
- **Stub screens** (one method per state in `app.cpp`), sharing two helpers in
  an anonymous namespace (`beginCenteredWindow`, `screenHeader`):
  - `updateMainMenu`: New Project / Load Project (disabled until the project
    browser exists) / Settings / Quit.
  - `updateProjectCreate`: placeholder outline of the Phase-5 fields + Cancel /
    Create & Run (jumps to Simulation for now).
  - `updateSimulation`: small info panel over the scene; grid renders only in
    this state.
  - `updateSettings`: "Show ImGui demo window" checkbox (development aid) +
    Back. All other preferences arrive in Phase 7.
- **Navigation semantics**: `ESC` is edge-triggered (a `m_wasEscPressed`
  latch means holding ESC does nothing extra) and acts as "back": it returns to
  the main menu from any screen and quits from the main menu. When ImGui is
  editing text or has a popup open, ESC is left to ImGui (clear field / close
  popup). ImGui keyboard nav stays enabled.
- The old Phase-1 `showImGuiDebugWindow` was removed; the demo window is now
  reachable from Settings.
- Verified: builds clean, `ctest` passes, app starts on the menu and runs.

### Phase 3 — Project model (DONE)

- **`src/core/project.{h,cpp}`** (new): pure-data `Project`, `MeshConfig`,
  `MaterialConfig`, `ProjectInfo` structs with `nlohmann::json` (de)serialisation.
  Includes `saveProject()`, `loadProject()`, `scanProjects()`, and the
  `nowIso8601()` timestamp helper (UTC ISO 8601).
- **`CMakeLists.txt`**: `core` now compiles `project.cpp` and links
  `nlohmann_json::nlohmann_json`.
- **`src/core/app.{h,cpp}`**:
  - `App` owns `m_projectsDir` (defaults to `./projects/` next to the binary),
    `m_recentProjects` (cached browser list), and `m_currentProject` (optional
    loaded project).
  - `init()` calls `scanRecentProjects()` after ImGui init.
  - New helpers: `scanRecentProjects()`, `saveCurrentProject()`, `loadProject()`,
    `initializeSimulationFromProject()` (rebuilds the grid to match the
    project's mesh config).
- **Menu browser** (`updateMainMenu`): lists projects with **Load** (loads,
  rebuilds grid, enters Simulation) and **Delete** (removes file, refreshes
  list) per row.
- **Project creation** (`updateProjectCreate`): real form fields for name,
  mesh (cells/extent), and Neo-Hookean material (μ, λ, ρ). **Create & Run**
  saves to `<projectsDir>/<name>.json`, loads it, initialises the grid, and
  transitions to Simulation.
- **Simulation overlay** (`updateSimulation`): shows current project name,
  **Save Project** button (writes modifiedAt + JSON), and **Back to Menu**.
- Projects are plain `.json` files — inspectable, editable, version-controllable.
- Verified: builds clean, `ctest` passes, full create → run → save → load cycle
  works; projects appear in the menu browser after restart.

### Phase 4 — Main menu browser (DONE)

Phase 4 is essentially merged into Phase 3 above: the project browser *is* the
menu implementation. The menu now shows a live list of saved projects with
functional Load / Delete per entry, and the New Project button opens the
creation form. No separate Phase-4 code was needed beyond what Phase 3 already
delivered.

### ImGui config directory (bonus)

- **`imgui_layer.cpp`**: the ImGui `.ini` file is now written to
  `~/.config/continuum2d/imgui.ini` (Linux) instead of the project root, so
  the repo stays clean. Falls back to CWD if `$HOME` is not set.

## Open questions (decision needed before/while building)

1. File dialog: use ImGuiFileDialog (header-only, cross-platform) vs hardcoded
   paths for MVP.
2. Projects directory: `./projects/` next to the binary vs platform standard
   (`~/.local/share/continuum2d/projects/`).
3. Auto-save on simulation exit, or manual save only.
4. Theme: ImGui Dark vs custom palette matching the simulation.
5. Settings persistence: remember window size, last project, theme.
