cmake_minimum_required(VERSION 3.16)

project(Sonoderm LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Threads REQUIRED)

include_directories(
    include
    vendor/imgui
    vendor/imgui/backends
)

add_executable(sonoderm
    src/glad.cpp
    src/Shader.cpp
    src/AudioEngine.cpp
    src/AudioAnalyzer.cpp
    src/MotionBrain.cpp
    src/SongDNA.cpp
    src/FeedbackBuffer.cpp
    src/GhostUI.cpp
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
    src/main.cpp
)

target_link_libraries(sonoderm PRIVATE
    OpenGL::GL
    glfw
    Threads::Threads
    ${CMAKE_DL_LIBS}
)
