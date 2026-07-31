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

### M2 — Render pipeline + test infrastructure
- `render/shader.{h,cpp}`: GLSL compile/link helper, loads from `shaders/`
- `render/mesh.{h,cpp}`: VAO/VBO wrapper over positions + triangle indices
- `render/camera.{h,cpp}`: 2D orthographic pan/zoom
- Extract window/loop from `main.cpp` into `core/app.{h,cpp}`
- Draw a triangle grid (future FEM mesh), wireframe + shaded
- Test scaffold: CMake library split + CTest + trivial Catch2 test

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
- [ ] M2 render pipeline + test infrastructure
- [ ] M3 kinematics
- [ ] M4 constitutive
- [ ] M5 nonlinear FEM solve
- [ ] M6 contact (pressure & shear)
