// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp


#include <glad/glad.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include <thread>
#include <queue>
#include <fstream>
#include <chrono>
#include <set>
#include <type_traits>
#include <concepts>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <implot.h>
#include <imgui_internal.h>

#include "Serial.h"
#include "Buffers.h"
#include "Monitor.h"
#include "FFT.h"
#include "tests.h"

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

float width = 1280, height = 720;
const int bauds[] = { 1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200, 230400, 250000, 460800, 500000, 921600, 1000000, 2000000 };
const int frecuencias[] = { 120, 240, 480, 960, 1440, 1920, 3840, 5760, 11520, 23040, 25000, 46080, 50000, 92160, 100000, 2000000 };

void window_resize(GLFWwindow* window, int w, int h) {
    width = w;
    height = h;
}

int MetricFormatter(double value, char* buff, int size, void* data) {
    const char* unit = (const char*)data;
    static double v[] = { 1e12, 1e9, 1e6, 1e3, 1, 1e-3, 1e-6, 1e-9, 1e-12};
    static const char* p[] = { "T", "G", "M", "k", "", "m", "u", "n", "p" };

    if (value == 0) {
        return snprintf(buff, size, "0 %s", unit);
    }

    for (int i = 0; i < 7; ++i) {
        if (fabs(value) >= v[i]) {
            return snprintf(buff, size, "%g %s%s", value / v[i], p[i], unit);
        }
    }
    return snprintf(buff, size, "%g %s%s", value / v[std::size(v) - 1], p[std::size(p) - 1], unit);
}

using namespace std::chrono_literals;
using namespace std::string_literals;

bool Button(const char* label, bool disabled = false) {
    if (disabled) {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    bool result = ImGui::Button(label);
    if (disabled) {
        ImGui::PopStyleVar();
        ImGui::PopItemFlag();
    }
    return result;
}

template <class Container, typename T>
void select_menu(const char* title, T & selection, const Container& values, std::function<std::string(T)> to_string, const char* empty_msg = "Vacio") {
    if (!ImGui::BeginMenu(title))
        return;

    if (std::empty(values)) {
        ImGui::Text(empty_msg);
        return;
    }

    bool selected = true;
    for (const auto &value : values){
        auto str = to_string(value);
        if (ImGui::MenuItem(str.c_str(), nullptr, value == selection ? &selected : nullptr)) {
            selection = value;
        }
    }

    ImGui::EndMenu();
}

template <class Container, typename T>
void combo(const char* title, T & selection, const Container& values, std::function<std::string(T)> to_string, const char* empty_msg = "Vacio") {
    auto str = to_string(selection);
    if (!ImGui::BeginCombo(title, str.c_str()))
        return;

    if (std::empty(values)) {
        ImGui::Text(empty_msg);
        return;
    }

    for (const auto &value : values){
        auto str = to_string(value);
        bool selected = value == selection;
        if (ImGui::Selectable(str.c_str(), &selected)) {
            selection = value;
        }
    }

    ImGui::EndCombo();
}

void MenuPuertos(std::string& selected_port) {
    auto puertos = EnumerateComPorts();
    std::function to_string = [](std::string s) { return s; };

    select_menu("Puerto", selected_port, puertos, to_string, "No hay ningún dispositivo conectado");
}

void MenuBaudRate(int& selected) {
    std::function to_string = [](int n) { return std::to_string(n); };

    select_menu("Velocidad", selected, bauds, to_string);
}

void ComboFrecuenciaMuestreo(int& selected) {
    std::function to_string = [](int n) { return std::to_string(n); };

    combo("Frecuencia", selected, frecuencias, to_string);
}

void ComboPuertos(std::string& selected_port) {
    auto puertos = EnumerateComPorts();
    std::function to_string = [](std::string s) { return s; };

    combo("Puerto", selected_port, puertos, to_string, "No hay ningún dispositivo conectado");
}

void ComboBaudRate(int& selected) {
    std::function to_string = [](int n) { return std::to_string(n); };
    
    int selection = selected;
    combo("Velocidad", selected, bauds, to_string);
}

struct Configuracion {
    int frecuencia_muestreo = 960;
    int baud_rate = 9600;
    int muestras = frecuencia_muestreo;
    std::string puerto;

    bool abierta = false;
};

class VentanaConfiguracion {
    Configuracion& config;
    bool& abierta;

public:
    VentanaConfiguracion(Configuracion& config) : config(config), abierta(config.abierta)
    {
    }

    void Abrir() {
        abierta = !abierta;
    }

    void Dibujar() {
        if (!abierta)
            return;

        ImGui::Begin("Configuración", &abierta);

        ComboFrecuenciaMuestreo(config.frecuencia_muestreo);
        ComboBaudRate(config.baud_rate);
        ComboPuertos(config.puerto);

        ImGui::End();
    }
};

struct Plotter {
    using clock = std::chrono::high_resolution_clock;
    using time_point = clock::time_point;
    using duration = std::chrono::duration<double>;

    Monitor monitor = Monitor(16 * 1024, 4 * 1024, 1024);

    FFT *fft = nullptr;
    ScrollBuffer<double>* scrollX = nullptr, * scrollY = nullptr,
        * out_scrollX = nullptr, * out_scrollY = nullptr;
    float* xs = nullptr, * ys = nullptr;

    int max_time = 60;
    int read_frequency = 30;
    float factor = 1;

    float show_time = 5 * factor;
    float offset = 0;

    float max_time_visible = 10 * factor;
    float max_time_scale = 10.0f;
    float min_time_scale = 5e-4;

    time_point start_time = clock::now();

    bool avanzar = true;
    int draw_size = 0;
    int maximo = 0, minimo = 255;

    int max = 0;

    double left_limit = 0, right_limit = max_time_visible;
    double ymin = -7, ymax = 7;
    double* linked_xmin = &left_limit, * linked_xmax = &right_limit;
    double* linked_ymin = &ymin, * linked_ymax = &ymax;

    float next_time = 0;

    size_t size = 0;

    std::vector<uint8_t> read_buffer, write_buffer;

    Configuracion& config;
    VentanaConfiguracion& ventanaConfig;
    std::function<double(double)> filtro;

    Plotter(Configuracion& config, VentanaConfiguracion& ventanaConfig) :
        config(config), ventanaConfig(ventanaConfig)
    {
        CrearBuffers();
        ConfigurarMonitor();
    }

    ~Plotter()
    {
        monitor.stop();
        DestruirBuffers();
    }

    void CrearBuffers() {
        int speed = config.frecuencia_muestreo;
        int max_size = speed * max_time;
        size = 0;
        int view_size = max_time_visible * speed;
        next_time = 0;

        DestruirBuffers();
        read_buffer.resize(1024);
        write_buffer.resize(1024);

        fft = new FFT(config.frecuencia_muestreo);
        scrollX = new ScrollBuffer<double>(max_size, view_size);
        scrollY = new ScrollBuffer<double>(max_size, view_size);
        out_scrollX = new ScrollBuffer<double>(max_size, view_size);
        out_scrollY = new ScrollBuffer<double>(max_size, view_size);
        xs = new float[view_size];
        ys = new float[view_size];
    }

    void DestruirBuffers() {
        delete scrollX;
        delete scrollY;
        delete out_scrollX;
        delete out_scrollY;
        delete[] xs;
        delete[] ys;
    }

    double transformacion (uint8_t v) {
        return (v - minimo) / (double)(maximo - minimo) * 12 - 6;
    };

    int transformacion_inversa (double v) {
        double resultado = round((v + 6) * (maximo - minimo) / 12.0 + minimo);
        if (resultado < 0)
            return 0;
        if (resultado > 255)
            return 255;
        return (int)resultado;
    };

    void ConfigurarMonitor() {
        monitor.read_callback = [&](int available) {
            int read = monitor.read(read_buffer.data(), read_buffer.size());

            for (size_t i = 0; i < read; i++)
            {
                double transformado = transformacion(read_buffer[i]);

                scrollY->push(transformado);
                scrollX->push(next_time);

                double resultado = filtro ? transformado : filtro(transformado);

                // Filtro
                out_scrollY->push(resultado);
                out_scrollX->push(next_time);
                next_time += 1.0f / config.frecuencia_muestreo;

                write_buffer[i] = transformacion_inversa(resultado);
            }

            monitor.write(write_buffer.data(), read);

            size = scrollX->count();
        };
    }

    void Conectar() {
        if (!monitor.started()) {
            if (config.puerto.empty())
                return;

            left_limit = 0, right_limit = max_time_visible;
            CrearBuffers();
            monitor.start(config.puerto.c_str(), config.baud_rate);
            start_time = clock::now();
            linked_xmin = linked_xmax = nullptr;
        }
        else {
            monitor.stop();
            linked_xmin = &left_limit;
            linked_xmax = &right_limit;
        }
    }

    void Dibujar() {
        static double last_time = glfwGetTime();
        static int stride_exp = 2, stride = 4;
        static double elapsed_time = 0;
        int byte_stride = sizeof(double) * stride;

        double now = glfwGetTime();
        double frame_time = (now - last_time) * 1000;
        last_time = now;

        draw_size = size / stride;
        if (monitor.started()) {
            elapsed_time = duration(clock::now() - start_time).count();

            if (elapsed_time > max_time_visible) {
                right_limit = scrollX->back();
                left_limit = right_limit - max_time_visible;
            }
        }

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ width, height });
        ImGui::Begin("Ventana principal", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus);

        if (ImGui::BeginMenuBar())
        {
            MenuPuertos(config.puerto);
            MenuBaudRate(config.baud_rate);

            if (ImGui::Button("Configuración"))
                ventanaConfig.Abrir();

            if (Button(monitor.started() ? "Desconectar" : "Conectar", config.puerto.empty())) {
                Conectar();
            }
            if (config.puerto.empty()) {
                ImGui::SetItemTooltip("Selecciona un dispositivo primero");
            }
            ImGui::EndMenuBar();
        }

        ImGui::Text("Tiempo transcurrido: %.1fs", elapsed_time);
        ImGui::Text("Application average %.2f ms/frame (%.1f FPS)", frame_time, 1000 / frame_time);
        if (ImGui::SliderInt("Stride", &stride_exp, 0, 10)) {
            stride = exp2(stride_exp);
        }

        if (ImPlot::BeginAlignedPlots("Grupo")) {
            if (ImPlot::BeginPlot("Entrada", { -1,0 }, ImPlotFlags_NoLegend)) {
                ImPlot::SetupAxisLinks(ImAxis_X1, linked_xmin, linked_xmax);
                ImPlot::SetupAxisLinks(ImAxis_Y1, linked_ymin, linked_ymax);

                ImPlot::SetupAxisFormat(ImAxis_Y1, MetricFormatter, (void*)"V");
                ImPlot::SetupAxisFormat(ImAxis_X1, MetricFormatter, (void*)"s");

                ImPlot::SetupAxisLimits(ImAxis_Y1, -7, 7, ImGuiCond_FirstUseEver);
                ImPlot::SetupAxisLimits(ImAxis_X1, left_limit, right_limit, monitor.started() ? ImGuiCond_Always : ImGuiCond_None);

                ImPlot::PlotLine("", scrollX->data(), scrollY->data(), draw_size, 0, 0, byte_stride);
                ImPlot::EndPlot();
            }

            if (ImGui::CollapsingHeader("Filtro")) {
                ImGui::Text("Sin implementar");
            }

            ImPlot::EndAlignedPlots();
        }


        if (ImGui::CollapsingHeader("Análisis")) {
            if (ImPlot::BeginPlot("Espectro", { -1, 0 }, ImPlotFlags_NoLegend)) {
                std::async([&] {
                    if (!fft || !scrollY)
                        return;

                    size_t available = scrollY->count();
                    size_t max = config.frecuencia_muestreo;
                    size_t count = available > max ? max : available;

                    auto end = scrollY->data() + available;
                    fft->SetData(end - count, count);
                    fft->Compute();
                });

                ImPlot::SetupAxisFormat(ImAxis_Y1, MetricFormatter, (void*)"V");
                ImPlot::SetupAxisFormat(ImAxis_X1, MetricFormatter, (void*)"Hz");
                ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_AutoFit);

                //ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
                //ImPlot::SetupAxisScale(ImAxis_Y1, scale, inverse_scale);

                fft->Draw(config.frecuencia_muestreo);
                ImPlot::EndPlot();
            }
        }

        ImGui::SliderInt("Máximo", &maximo, 0, 256);
        ImGui::SliderInt("Mínimo", &minimo, 0, 256);

        ImGui::Text("Application average %.2f ms/frame (%.1f FPS)", frame_time, 1000 / frame_time);
        ImGui::End();
    }
};


// Main code
int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.3 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Procesamiento Digital de Señales", nullptr, nullptr);
    if (window == nullptr)
        return 1;

    glfwSetWindowSizeCallback(window, window_resize);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    if (!gladLoadGL()) {
        glfwTerminate();
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    //std::cout << std::format("Factor {} Max {} Min {} Show_time {}\n", factor, max_time_scale, min_time_scale, show_time);

    int speed, max_size, view_size = 0;

    Configuracion config;
    VentanaConfiguracion ventanaConfig(config);

    Plotter plotter(config, ventanaConfig);
    plotter.filtro = [](double d) {
        return d / 2;
    };

    //std::ofstream log("C:/Users/usuario/Desktop/log.txt");

    ImPlotTransform scale = [](double v, void*) {
        return sqrt(v);
    };

    ImPlotTransform inverse_scale = [](double value, void*) {
        return value * value;
    };

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            plotter.Dibujar();
            ventanaConfig.Dibujar();
        }

        glViewport(0, 0, width, height);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
