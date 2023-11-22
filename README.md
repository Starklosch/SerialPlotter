# SerialPlotter

Este es un programa simple de DSP que intercambia información de señales con un microcontrolador mediante el puerto serie de la computadora.

## Características
- Obtiene una señal muestreada por el microcontrolador y la muestra.
- Analiza la señal a partir de un FFT y dibuja sus armónicas.
- Aplica un filtro a la señal, la grafica y envía de regreso al microcontrolador.

## Librerías utilizadas
- [GLFW](https://github.com/glfw/glfw)
- [glad](https://github.com/Dav1dde/glad)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [ImPlot](https://github.com/epezent/implot)
- [FFTW](https://fftw.org/)
- [DSP IIR Realtime C++ filter library](https://github.com/berndporr/iir1)

## Problemas conocidos
- Cuando se arrastra o se cambia el estado de la ventana se produce un pequeño desfase.
