cmake_minimum_required(VERSION 3.14)
project(SpriteSheetSlicer VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FetchContent for downloading dependencies
include(FetchContent)

# Download GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Find OpenGL
find_package(OpenGL REQUIRED)

# ImGui source files
set(IMGUI_DIR ${PROJECT_SOURCE_DIR}/external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/external)
include_directories(${IMGUI_DIR})
include_directories(${IMGUI_DIR}/backends)

# Source files
set(SOURCES
    src/main.cpp
    external/tinyfiledialogs.c
    ${IMGUI_SOURCES}
)

# Create executable
add_executable(spritesheet_slicer ${SOURCES})

# Link libraries
target_link_libraries(spritesheet_slicer
    glfw
    OpenGL::GL
    ${CMAKE_DL_LIBS}
)

# Add compiler warnings
if(MSVC)
    target_compile_options(spritesheet_slicer PRIVATE /W4)
else()
    target_compile_options(spritesheet_slicer PRIVATE -Wall -Wextra -pedantic)
endif()
