# Importar iir1
add_subdirectory(iir1)

# Importar GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
#set(ENABLE_AVX2 ON CACHE BOOL "" FORCE)
add_subdirectory(fftw3)
file(COPY_FILE fftw3/api/fftw3.h ../include/fftw3.h)

# Importar Imgui
file(GLOB imgui_sources imgui/*.h imgui/*.cpp)

add_library(imgui
        ${imgui_sources}
        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_opengl3.cpp)

target_include_directories(imgui PUBLIC
        imgui
        imgui/backends)

target_link_libraries(imgui PRIVATE
        glfw)

# Importar Implot
file(GLOB implot_sources implot/*.h implot/*.cpp)

add_library(implot
        ${implot_sources})

target_include_directories(implot PUBLIC
        implot)

target_link_libraries(implot imgui)