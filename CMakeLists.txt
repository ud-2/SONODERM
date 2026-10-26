cmake_minimum_required(VERSION 3.16)

project(Sonoderm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Threads REQUIRED)

include_directories(include)
add_executable(sonoderm
    src/glad.cpp
    src/Shader.cpp
    src/AudioEngine.cpp
    src/AudioAnalyzer.cpp
    src/MotionBrain.cpp
    src/SongDNA.cpp
    src/FeedbackBuffer.cpp
    src/main.cpp
)

target_link_libraries(sonoderm PRIVATE
    OpenGL::GL
    glfw
    Threads::Threads
    ${CMAKE_DL_LIBS} # Required for dynamic audio driver loading
)