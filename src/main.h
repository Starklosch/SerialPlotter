#include <thread>
#include <queue>
#include <fstream>
#include <chrono>
#include <set>
#include <type_traits>
#include <concepts>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <implot.h>
#include <imgui_internal.h>
#include <Iir.h>

#include "Serial.h"
#include "Buffers.h"
#include "FFT.h"
