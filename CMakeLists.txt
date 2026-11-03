cmake_minimum_required(VERSION 3.14)
project(MeshAnalysisToolkit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(meshcore
  src/mesh.cpp
  src/loader.cpp
  src/report.cpp)
target_include_directories(meshcore PUBLIC include)
if(MSVC)
  target_compile_options(meshcore PRIVATE /W4)
else()
  target_compile_options(meshcore PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(meshtool src/main.cpp)
target_link_libraries(meshtool PRIVATE meshcore)

option(MESH_BUILD_TESTS "Build the GoogleTest suite" ON)
if(MESH_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  add_executable(mesh_tests tests/test_mesh.cpp)
  target_link_libraries(mesh_tests PRIVATE meshcore GTest::gtest_main)
  target_compile_definitions(mesh_tests PRIVATE SAMPLES_DIR="${CMAKE_SOURCE_DIR}/samples")
  include(GoogleTest)
  gtest_discover_tests(mesh_tests)
endif()
