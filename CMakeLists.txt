cmake_minimum_required(VERSION 3.16)
project(continuum2d LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Catch2 REQUIRED)
find_package(nlohmann_json REQUIRED)

include(FetchContent)

# Dear ImGui: fetched from GitHub (no apt package is used). The upstream repo
# ships no CMakeLists.txt, so after FetchContent populates the sources we build
# the core library and the GLFW + OpenGL3 backends into an `imgui` target.
# Requires network access on the first configure.
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.9
  GIT_SHALLOW TRUE
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
  add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  )
  target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
  )
  target_link_libraries(imgui PUBLIC glfw OpenGL::GL)
endif()

# Header-only math wrappers (GLSL-compatible GLM helpers). Carries the `src`
# include path to every target.
add_library(math INTERFACE)
target_include_directories(math INTERFACE src)

# Continuum mechanics: mesh, deformation, material, FEM, contact.
# Must never be included by render/.
add_library(sim STATIC src/sim/sim.cpp)
target_link_libraries(sim PUBLIC math Eigen3::Eigen)
target_compile_options(sim PRIVATE -Wall -Wextra)

# OpenGL rendering: shaders, meshes, camera. Must never include sim/.
add_library(render STATIC
  src/render/shader.cpp
  src/render/mesh.cpp
  src/render/camera.cpp
)
target_link_libraries(render PUBLIC math glm::glm GLEW::GLEW)
target_compile_options(render PRIVATE -Wall -Wextra)

# Application shell: window, input, main loop, GUI. Owns the window and the GL
# context (hence GLFW/GLEW/OpenGL); glues render + sim together.
add_library(core STATIC src/core/app.cpp src/core/imgui_layer.cpp)
target_link_libraries(core PUBLIC render OpenGL::GL glfw GLEW::GLEW imgui)
target_compile_options(core PRIVATE -Wall -Wextra)

add_executable(continuum2d src/main.cpp)
target_link_libraries(continuum2d PRIVATE core)
target_compile_options(continuum2d PRIVATE -Wall -Wextra)

# Headless tests: sim + math only, never render/GLFW/GLEW.
add_executable(sim_tests tests/sim_tests.cpp)
target_link_libraries(sim_tests PRIVATE sim Catch2::Catch2WithMain)
target_compile_options(sim_tests PRIVATE -Wall -Wextra)

include(CTest)
if(BUILD_TESTING)
  add_test(NAME sim_tests COMMAND sim_tests)
endif()
