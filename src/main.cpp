// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "main.h"


float width = 1280, height = 720;
const int bauds[] = { 1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200, 230400, 250000, 460800, 500000, 921600, 1000000, 2000000 };
const int frecuencias[] = { 120, 240, 480, 960, 1440, 1920, 3840, 5760, 11520, 23040, 25000, 46080, 50000, 92160, 100000, 2000000 };

void window_resize(GLFWwindow* window, int w, int h) {
    width = w;
    height = h;
}

bool minimized = false;
void window_minimized(GLFWwindow* window, int _minimized) {
    minimized = _minimized;
}

bool focused = true;
void window_focused(GLFWwindow* window, int _focused) {
    focused = _focused;
}

std::string MetricFormatter(double value, std::string_view unit) {
    static double v[] = { 1e12, 1e9, 1e6, 1e3, 1, 1e-3, 1e-6, 1e-9, 1e-12 };
    static const char* p[] = { "T", "G", "M", "k", "", "m", "u", "n", "p" };

    if (value == 0) {
        return "";
    }

    for (int i = 0; i < std::size(p); ++i) {
        if (fabs(value) >= v[i]) {
            return std::format("{:g} {}{}", value / v[i], p[i], unit);
        }
    }
}

int MetricFormatter(double value, char* buff, int size, void* data) {
    const char* unit = (const char*)data;
    static double v[] = { 1e12, 1e9, 1e6, 1e3, 1, 1e-3, 1e-6, 1e-9, 1e-12 };
    static const char* p[] = { "T", "G", "M", "k", "", "m", "u", "n", "p" };

    if (value == 0) {
        return snprintf(buff, size, "0 %s", unit);
    }

    for (int i = 0; i < std::size(p); ++i) {
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
void select_menu(const char* title, T& selection, const Container& values, std::function<std::string(T)> to_string, const char* empty_msg = "Vacio") {
    if (!ImGui::BeginMenu(title))
        return;

    if (std::empty(values)) {
        ImGui::Text(empty_msg);
        return;
    }

    bool selected = true;
    for (const auto& value : values) {
        auto str = to_string(value);
        if (ImGui::MenuItem(str.c_str(), nullptr, value == selection ? &selected : nullptr)) {
            selection = value;
        }
    }

    ImGui::EndMenu();
}

template <class Container, typename T>
void select_menu2(const char* title, T& selection, std::function<Container()> get_values, std::function<std::string(T)> to_string, const char* empty_msg = "Vacio") {
    if (!ImGui::BeginMenu(title))
        return;

    Container values = get_values();
    if (std::empty(values)) {
        ImGui::Text(empty_msg);
        return;
    }

    bool selected = true;
    for (const auto& value : values) {
        auto str = to_string(value);
        if (ImGui::MenuItem(str.c_str(), nullptr, value == selection ? &selected : nullptr)) {
            selection = value;
        }
    }

    ImGui::EndMenu();
}

template <class Container, typename T>
void combo(const char* title, T& selection, const Container& values, std::function<std::string(T)> to_string, const char* empty_msg = "Vacio") {
    auto str = to_string(selection);
    if (!ImGui::BeginCombo(title, str.c_str()))
        return;

    if (std::empty(values)) {
        ImGui::Text(empty_msg);
        return;
    }

    for (const auto& value : values) {
        auto str = to_string(value);
        bool selected = value == selection;
        if (ImGui::Selectable(str.c_str(), &selected)) {
            selection = value;
        }
    }

    ImGui::EndCombo();
}

void MenuPuertos(std::string& selected_port) {
    //auto puertos = EnumerateComPorts();
    std::function to_string = [](std::string s) { return s; };

    select_menu2("Puerto", selected_port, std::function(EnumerateComPorts), to_string, "No hay ningún dispositivo conectado");
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

struct Settings {
    int sampling_rate = 3840;
    int baud_rate = sampling_rate * 10;
    int samples = sampling_rate;
    std::string port = "COM5";

    int maximum = 49, minimum = 175;
    int stride = 4;
    int byte_stride = sizeof(double) * stride;

    double map_factor = 12.0 / (maximum - minimum);

    bool show_frame_time = false;
    bool open = false;
};

class SettingsWindow {
    Settings& settings;
    bool& open;

public:
    SettingsWindow(Settings& settings) : settings(settings), open(settings.open)
    {
    }

    void Toggle() {
        open = !open;
    }

    void Draw() {
        using namespace ImGui;
        static int stride_exp = 2;

        if (!open)
            return;

        Begin("Configuración", &open);

        ComboFrecuenciaMuestreo(settings.sampling_rate);
        ComboBaudRate(settings.baud_rate);
        ComboPuertos(settings.port);

        settings.samples = settings.sampling_rate;

        if (TreeNode("Mapeo")) {
            SliderInt("Máximo", &settings.maximum, 0, 255);
            SliderInt("Mínimo", &settings.minimum, 0, 255);
            settings.map_factor = 12.0 / (settings.maximum - settings.minimum);
            TreePop();
        }

        if (TreeNode("Rendimiento")) {
            if (SliderInt("Stride", &stride_exp, 0, 10)) {
                settings.stride = exp2(stride_exp);
                settings.byte_stride = sizeof(double) * settings.stride;
            }
            SetItemTooltip("Dibuja 1 de cada 2^n muestras para mejorar el rendimiento.");

            Checkbox("Mostrar tiempo de renderizado", &settings.show_frame_time);
            TreePop();
        }

        End();
    }
};

Iir::Butterworth::LowPass<8> lowpass_filter;
Iir::Butterworth::HighPass<8> highpass_filter;

class MainWindow {
    using clock = std::chrono::high_resolution_clock;
    using time_point = clock::time_point;
    using duration = std::chrono::duration<double>;

    enum class Filter {
        None,
        LowPass,
        HighPass
    };

    Serial serial;
    std::thread serial_thread, analysis_thread;
    std::mutex analysis_mutex;
    std::condition_variable analysis_cv;

    FFT* fft = nullptr;
    ScrollBuffer<double>* scrollX = nullptr, * scrollY = nullptr, * filter_scrollY = nullptr;

    int max_time = 120;
    int read_frequency = 30;

    float offset = 0;

    float max_time_visible = 5;

    time_point start_time = clock::now();

    int max = 0;

    // Límites del gráfico de entrada y filtrado
    double left_limit = 0, right_limit = max_time_visible;
    double down_limit = -7, up_limit = 7;

    double next_time = 0;

    // Cantidad de puntos a dibujar
    size_t size = 0;

    std::vector<uint8_t> read_buffer, write_buffer;

    Settings& settings;
    SettingsWindow& settingsWindow;

    int min_cutoff_frequency = 1, max_cutoff_frequency = 100;
    int cutoff_frequency[3] = { 0, 20, 100 };
    Filter selected_filter = Filter::LowPass;

public:
    MainWindow(Settings& config, SettingsWindow& ventanaConfig) :
        settings(config), settingsWindow(ventanaConfig)
    {
        CreateBuffers();

        lowpass_filter.setup(settings.sampling_rate, cutoff_frequency[1]);
        highpass_filter.setup(settings.sampling_rate, cutoff_frequency[2]);
    }

    ~MainWindow()
    {
        Stop();
        DestroyBuffers();
    }

private:
    void CreateBuffers() {
        int speed = settings.sampling_rate;
        int max_size = speed * max_time;
        size = 0;
        int view_size = 30 * speed;
        next_time = 0;

        DestroyBuffers();
        read_buffer.resize(128);
        write_buffer.resize(128);

        fft = new FFT(settings.sampling_rate);
        scrollX = new ScrollBuffer<double>(max_size, view_size);
        scrollY = new ScrollBuffer<double>(max_size, view_size);
        filter_scrollY = new ScrollBuffer<double>(max_size, view_size);
    }

    void DestroyBuffers() {
        delete scrollX;
        delete scrollY;
        delete filter_scrollY;
    }

    double TransformSample(uint8_t v) {
        return (v - settings.minimum) * settings.map_factor - 6;
    };

    uint8_t InverseTransformSample(double v) {
        double result = round((v + 6) * (settings.maximum - settings.minimum) / 12.0 + settings.minimum);
        if (result < 0)
            return 0;
        if (result > 255)
            return 255;
        return (int)result;
    };

    bool started = false;
    void ToggleConnection() {
        if (!started) {
            if (settings.port.empty())
                return;

            Start();
        }
        else {
            Stop();
        }
        started = !started;
    }

    void Start() {
        CreateBuffers();

        left_limit = 0, right_limit = max_time_visible;

        // Reiniciar filtro
        lowpass_filter.reset();
        highpass_filter.reset();

        // Iniciar serial e hilos
        do_serial_work = true;
        do_analysis_work = true;
        analysis_thread = std::thread(&MainWindow::AnalysisWorker, this);

        serial.open(settings.port, settings.baud_rate);
        serial_thread = std::thread(&MainWindow::SerialWorker, this);
        start_time = clock::now();
    }

    void Stop() {
        do_serial_work = false;
        do_analysis_work = false;
        analysis_cv.notify_one();

        if (serial_thread.joinable())
            serial_thread.join();
        if (analysis_thread.joinable())
            analysis_thread.join();
        serial.close();
    }

    void SelectFilter(Filter filter) {
        selected_filter = filter;
        switch (selected_filter)
        {
        case Filter::LowPass:
            min_cutoff_frequency = 1;
            max_cutoff_frequency = settings.sampling_rate / 4;
            break;
        case Filter::HighPass:
            min_cutoff_frequency = settings.sampling_rate / 4;
            max_cutoff_frequency = settings.sampling_rate / 2 - 1;
            break;
        }
    }

    void SetupFilter() {
        switch (selected_filter)
        {
        case Filter::LowPass:
            lowpass_filter.setup(settings.sampling_rate, cutoff_frequency[1]);
            break;
        case Filter::HighPass:
            highpass_filter.setup(settings.sampling_rate, cutoff_frequency[2]);
            break;
        }
    }

    void ResetFilters() {
        lowpass_filter.reset();
        highpass_filter.reset();
    }

    bool do_serial_work = true;
    bool filter_open = false;
    void SerialWorker() {
        while (do_serial_work) {
            // Es importante trabajar con muy pocas muestras.
            // Si trabajamos con todas las que están disponibles se produce un tiempo muerto.
            int read = serial.read(read_buffer.data(), 1);

            for (size_t i = 0; i < read; i++)
            {
                double transformado = TransformSample(read_buffer[i]);

                scrollY->push(transformado);
                scrollX->push(next_time);

                double resultado = transformado;

                switch (selected_filter)
                {
                case Filter::LowPass:
                    resultado = lowpass_filter.filter(transformado);
                    break;
                case Filter::HighPass:
                    resultado = highpass_filter.filter(transformado);
                    break;
                }

                filter_scrollY->push(resultado);
                next_time += 1.0 / settings.sampling_rate;

                write_buffer[i] = 255 - InverseTransformSample(resultado);
            }

            serial.write(write_buffer.data(), read);
            size = scrollX->count();
        }
    }

    bool do_analysis_work = true;
    void AnalysisWorker() {
        while (do_analysis_work) {
            std::unique_lock lock(analysis_mutex);
            analysis_cv.wait(lock);

            if (!fft || !scrollY)
                continue;

            size_t available = scrollY->count();
            size_t max = settings.sampling_rate;
            size_t count = available > max ? max : available;

            auto end = scrollY->data() + available;
            fft->SetData(end - count, count);
            fft->Compute();

            std::this_thread::sleep_for(100ms);
        }
    }

    float statusbar_height = 30;
public:
    void Draw() {
        static double last_time = glfwGetTime();
        static double elapsed_time = 0;

        double now = glfwGetTime();
        last_time = now;

        double draw_size = size / settings.stride;
        if (started) {
            elapsed_time = duration(clock::now() - start_time).count();

            if (elapsed_time > max_time_visible) {
                right_limit = scrollX->back();
                left_limit = right_limit - max_time_visible;
            }
            offset = left_limit;
        }

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ width, height - statusbar_height });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::Begin("Ventana principal", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar();

        if (ImGui::BeginMenuBar())
        {
            MenuPuertos(settings.port);
            MenuBaudRate(settings.baud_rate);

            if (ImGui::Button("Configuración"))
                settingsWindow.Toggle();

            if (Button(started ? "Desconectar" : "Conectar", settings.port.empty())) {
                ToggleConnection();
            }
            if (settings.port.empty()) {
                ImGui::SetItemTooltip("Selecciona un dispositivo primero");
            }
            ImGui::EndMenuBar();
        }

        if (ImPlot::BeginPlot("Entrada", { -1,0 }, ImPlotFlags_NoLegend)) {
            // Los cambios de posición en una gráfica tienen efectos sobre la otra
            ImPlot::SetupAxisLinks(ImAxis_X1, &left_limit, &right_limit);
            ImPlot::SetupAxisLinks(ImAxis_Y1, &down_limit, &up_limit);

            ImPlot::SetupAxisFormat(ImAxis_Y1, MetricFormatter, (void*)"V");
            ImPlot::SetupAxisFormat(ImAxis_X1, MetricFormatter, (void*)"s");

            ImPlot::SetupAxisLimits(ImAxis_Y1, -7, 7, ImGuiCond_FirstUseEver);
            ImPlot::SetupAxisLimits(ImAxis_X1, left_limit, right_limit, started ? ImGuiCond_Always : ImGuiCond_None);

            ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 0, INFINITY);

            ImPlot::PlotLine("", scrollX->data(), scrollY->data(), draw_size, 0, 0, settings.byte_stride);
            ImPlot::EndPlot();
        }

        filter_open = ImGui::CollapsingHeader("Filtro");
        if (filter_open) {
            if (ImPlot::BeginPlot("Salida", { -1,0 }, ImPlotFlags_NoLegend)) {
                ImPlot::SetupAxisLinks(ImAxis_X1, &left_limit, &right_limit);
                ImPlot::SetupAxisLinks(ImAxis_Y1, &down_limit, &up_limit);

                ImPlot::SetupAxisFormat(ImAxis_Y1, MetricFormatter, (void*)"V");
                ImPlot::SetupAxisFormat(ImAxis_X1, MetricFormatter, (void*)"s");

                ImPlot::SetupAxisLimits(ImAxis_Y1, -7, 7, ImGuiCond_FirstUseEver);
                ImPlot::SetupAxisLimits(ImAxis_X1, left_limit, right_limit, started ? ImGuiCond_Always : ImGuiCond_None);

                ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 0, INFINITY);

                ImPlot::PlotLine("", scrollX->data(), filter_scrollY->data(), draw_size, 0, 0, settings.byte_stride);
                ImPlot::EndPlot();
            }

            const char* nombres[] = {"Ninguno", "Pasa bajos", "Pasa altos"};
            for (int i = 0; i < std::size(nombres); i++)
            {
                if (i > 0)
                    ImGui::SameLine();

                if (i == (int)selected_filter) {
                    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0xFFfa9642));
                    ImGui::Button(nombres[i]);
                    ImGui::PopStyleColor();
                }
                else {
                    if (ImGui::Button(nombres[i])) {
                        SelectFilter((Filter)i);
                        ResetFilters();
                    }
                }
            }


            if (selected_filter != Filter::None
                && ImGui::SliderInt("Frecuencia de corte", &cutoff_frequency[(int)selected_filter], min_cutoff_frequency, max_cutoff_frequency)){
                SetupFilter();
                ResetFilters();
            }
        }

        if (ImGui::CollapsingHeader("Análisis")) {
            analysis_cv.notify_one();
            if (ImPlot::BeginPlot("Espectro", { -1, 0 }, ImPlotFlags_NoLegend)) {
                ImPlot::SetupAxisFormat(ImAxis_Y1, MetricFormatter, (void*)"V");
                ImPlot::SetupAxisFormat(ImAxis_X1, MetricFormatter, (void*)"Hz");
                ImPlot::SetupAxisLimits(ImAxis_X1, 0.99, settings.samples, ImGuiCond_FirstUseEver);
                ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_AutoFit);

                ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
                ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0, INFINITY);

                fft->Plot(settings.sampling_rate);
                ImPlot::EndPlot();

                // Mostrar información adicional si hay datos
                if (scrollY && scrollY->count() > 0) {
                    ImGui::Text("Frecuencia: %s\tDesplazamiento %s",
                        MetricFormatter(fft->Frequency(settings.sampling_rate), "Hz").data(),
                        MetricFormatter(fft->Offset(), "V").data()
                    );
                }
            }
        }

        // Barra inferior
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        if (ImGui::BeginViewportSideBar("Status", 0, ImGuiDir_Down, statusbar_height, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("Tiempo transcurrido: %.1fs", elapsed_time);
            if (settings.show_frame_time) {
                int max = ImGui::GetContentRegionMax().x;
                int available = max - ImGui::GetItemRectSize().x;

                std::string info = std::format("Rendimiento: {:.1f} ms/frame ({:.1f} FPS)", 1000.0f / io.Framerate, io.Framerate);
                auto info_size = ImGui::CalcTextSize(info.c_str());

                ImGui::SameLine(ImGui::GetContentRegionMax().x - info_size.x);
                ImGui::Text(info.c_str());
            }
            ImGui::End();
        }
        ImGui::PopStyleVar();

        ImGui::End();
    }
};

// Main code
int main(int, char**)
{
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
    glfwSetWindowIconifyCallback(window, window_minimized);
    glfwSetWindowFocusCallback(window, window_focused);
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

    ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    Settings settings;
    SettingsWindow settings_window(settings);

    MainWindow mainWindow(settings, settings_window);

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
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

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
