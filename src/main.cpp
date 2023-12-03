// Este archivo está basado en un ejemplo de Omar Cornut
// Fuente: https://github.com/ocornut/imgui/blob/master/examples/example_glfw_opengl3/main.cpp

// The MIT License (MIT)
//
// Copyright (c) 2014-2023 Omar Cornut

#include "main.h"
#include "Settings.h"
#include "MainWindow.h"
#include "Console.h"


int width = 1280, height = 720;

Settings settings;
SettingsWindow settings_window(settings);

MainWindow mainWindow(width, height, settings, settings_window);

void window_resize(GLFWwindow*, int w, int h) {
    width = w;
    height = h;
    mainWindow.SetSize(w, h);
}

bool minimized = false;
void window_minimized(GLFWwindow*, int _minimized) {
    minimized = _minimized;
}

bool focused = true;
void window_focused(GLFWwindow*, int _focused) {
    focused = _focused;
}

// Main code
int main(int, char**)
{
    // Ocultar consola
    Console console;
    if (console.IsOwn())
        console.Hide(true);

    if (!glfwInit())
        return -1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
        const char* glsl_version = "#version 100";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 330
        const char* glsl_version = "#version 330";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.3 + GLSL 330
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(width, height, "Procesamiento Digital de Señales", nullptr, nullptr);
    if (window == nullptr)
        return -1;

    glfwSetWindowSizeCallback(window, window_resize);
    glfwSetWindowIconifyCallback(window, window_minimized);
    glfwSetWindowFocusCallback(window, window_focused);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    if (!gladLoadGL()) {
        glfwTerminate();
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();

    ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    double last_time = glfwGetTime();
    int minimized_fps = 20;
    double minimized_frametime = 1.0 / minimized_fps;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Espera para procesar los eventos si la ventana está minimizada 
        if (minimized)
            glfwWaitEvents();
        else
            glfwPollEvents();

        // Limitar FPS y uso de CPU si la ventana está abierta pero no tiene foco
        if (!focused) {
            if (minimized_frametime > 0.02)
                std::this_thread::sleep_for(std::chrono::duration<double>(minimized_frametime - 0.02));

            double elapsed = glfwGetTime() - last_time;
            while (elapsed < minimized_frametime)
                elapsed = glfwGetTime() - last_time;
            last_time = glfwGetTime();
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        mainWindow.Draw();
        settings_window.Draw();

        glViewport(0, 0, width, height);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
