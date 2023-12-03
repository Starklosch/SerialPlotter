# SerialPlotter

Este es un programa simple de DSP que intercambia información de señales con un microcontrolador mediante el puerto serie de la computadora.

## Características
- Obtiene una señal muestreada por el microcontrolador y la muestra.
- Analiza la señal a partir de un FFT y dibuja sus armónicas.
- Aplica un filtro a la señal, la grafica y envía de regreso al microcontrolador.

## Librerías utilizadas
- [GLFW](https://github.com/glfw/glfw): A multi-platform library for OpenGL, OpenGL ES, Vulkan, window and input.
- [glad](https://github.com/Dav1dde/glad): Multi-Language Vulkan/GL/GLES/EGL/GLX/WGL Loader-Generator based on the official specs.
- [Dear ImGui](https://github.com/ocornut/imgui): Dear ImGui: Bloat-free Graphical User interface for C++ with minimal dependencies.
- [ImPlot](https://github.com/epezent/implot): Immediate Mode Plotting.
- [FFTW](https://fftw.org/): C subroutine library for computing the discrete Fourier transform (DFT) in one or more dimensions, of arbitrary input size, and of both real and complex data.
- [iir1](https://github.com/berndporr/iir1): DSP IIR realtime filter library written in C++.

## Compilación

    cmake -DCMAKE_BUILD_TYPE=Release -S ruta/al/proyecto -B build
    cmake --build build

## Problemas conocidos
- Cuando se arrastra o se cambia el estado de la ventana se produce un pequeño desfase.
