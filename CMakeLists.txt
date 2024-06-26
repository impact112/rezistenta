cmake_minimum_required(VERSION 3.5.0)
project(ReziEngine VERSION 0.7.0)

set(CXX_STANDARD 11)
set(CXX_STANDARD_REQUIRED ON)

if(MSVC)
    message(FATAL_ERROR "MSVC is unsupported.")
endif()

include(FetchContent)

if(NOT PLATFORM)
  set(PLATFORM Desktop)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

if (EMSCRIPTEN)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s GL_ENABLE_GET_PROC_ADDRESS=1 -fwasm-exceptions")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif ()

if ("${PLATFORM}" STREQUAL "Desktop")
    set(GRAPHICS "GRAPHICS_OPENGL_43")
    add_compile_options(-O2)
endif ()

set(RAYLIB_VERSION 5.0)
find_package(raylib ${RAYLIB_VERSION} QUIET)
if (NOT raylib_FOUND)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED)
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
  endif()
endif()

set(EIGEN_VERSION 3.4.0)
FetchContent_Declare(
  eigen
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG        ${EIGEN_VERSION}
)
FetchContent_GetProperties(eigen)
if(NOT eigen_POPULATED)
  FetchContent_Populate(eigen)
  add_library(Eigen3 INTERFACE)
  target_include_directories(Eigen3 INTERFACE ${eigen_SOURCE_DIR})
endif()
add_library(Eigen3::Eigen ALIAS Eigen3)

set(TOML11_VERSION 3.8.1)
FetchContent_Declare(
  toml11
  GIT_REPOSITORY https://github.com/ToruNiina/toml11.git
  GIT_TAG        "v${TOML11_VERSION}"
)
FetchContent_GetProperties(toml11)
if(NOT toml11_POPULATED)
  FetchContent_Populate(toml11)
  add_library(toml11 INTERFACE)
  target_include_directories(toml11 INTERFACE ${toml11_SOURCE_DIR})
endif()


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(libReziEngine)
add_subdirectory(ReziEdit)
if ("${PLATFORM}" STREQUAL "Desktop")
  add_subdirectory(ReziCLI)
endif()

if (APPLE)
    target_link_libraries(ReziEngine "-framework IOKit")
    target_link_libraries(ReziEngine "-framework Cocoa")
    target_link_libraries(ReziEngine "-framework OpenGL")
endif()

