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

add_executable(continuum2d
  src/main.cpp
)

target_include_directories(continuum2d PRIVATE src)

target_link_libraries(continuum2d PRIVATE
  OpenGL::GL
  glfw
  GLEW::GLEW
  glm::glm
  Eigen3::Eigen
)

target_compile_options(continuum2d PRIVATE -Wall -Wextra)
