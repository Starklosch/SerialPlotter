cmake_minimum_required(VERSION 3.20)
project(SerialPlotter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(BUILD_SHARED_LIBS FALSE CACHE BOOL "" FORCE)

add_subdirectory(extern)

add_executable(SerialPlotter
        glad.c
        src/main.cpp
        src/FFT.cpp
        src/MainWindow.cpp
        src/Serial.cpp
        src/Settings.cpp
        src/Console.cpp)

target_include_directories(SerialPlotter PUBLIC
        include)

# Vincular librerías
target_link_libraries(SerialPlotter PRIVATE
        glfw
        imgui
        implot
        iir::iir_static
        fftw3)
