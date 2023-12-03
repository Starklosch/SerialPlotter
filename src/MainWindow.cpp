#include <imgui.h>
#include <imgui_internal.h>

#include <implot.h>
#include <Iir.h>
#include <thread>

#include "MainWindow.h"

#include "Buffers.h"
#include "Settings.h"

const int bauds[] = { 1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200, 230400, 250000, 460800, 500000, 921600, 1000000, 2000000 };
const int frecuencias[] = { 120, 240, 480, 960, 1440, 1920, 3840, 5760, 11520, 23040, 25000, 46080, 50000, 92160, 100000, 2000000 };

#include "Widgets.h"

using namespace std::chrono_literals;

Iir::Butterworth::LowPass<8> lowpass_filter;
Iir::Butterworth::HighPass<8> highpass_filter;


void MenuPuertos(std::string& selected_port) {
    std::function to_string = [](std::string s) { return s; };

    select_menu("Puerto", selected_port, std::function(EnumerateComPorts), to_string,
                "No hay ningún dispositivo conectado");
}

bool Button(const char* label, bool disabled = false) {
    if (disabled) {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);

        ImGui::Button(label);

        ImGui::PopStyleVar();
        ImGui::PopItemFlag();
        return false;
    }
    return ImGui::Button(label);
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
    return "";
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


MainWindow::MainWindow(int width, int height, Settings& config, SettingsWindow& ventanaConfig) :
    settings(&config), settingsWindow(&ventanaConfig), width(width), height(height)
{
    CreateBuffers();
}

MainWindow::~MainWindow()
{
    Stop();
    DestroyBuffers();
}

void MainWindow::CreateBuffers() {
    int speed = settings->sampling_rate;
    int max_size = speed * max_time;
    size = 0;
    int view_size = 30 * speed;
    next_time = 0;

    DestroyBuffers();
    read_buffer.resize(128);
    write_buffer.resize(128);

    fft = new FFT(settings->sampling_rate);
    scrollX = new ScrollBuffer<double>(max_size, view_size);
    scrollY = new ScrollBuffer<double>(max_size, view_size);
    filter_scrollY = new ScrollBuffer<double>(max_size, view_size);
}

void MainWindow::DestroyBuffers() {
    delete scrollX;
    delete scrollY;
    delete filter_scrollY;
}

double MainWindow::TransformSample(uint8_t v) {
    return (v - settings->minimum) * settings->map_factor - 6;
}

uint8_t MainWindow::InverseTransformSample(double v) {
    double result = round((v + 6) * (settings->maximum - settings->minimum) / 12.0 + settings->minimum);
    if (result < 0)
        return 0;
    if (result > 255)
        return 255;
    return (int)result;
}


void MainWindow::ToggleConnection()
{
    if (!started) {
        if (settings->port.empty())
            return;

        Start();
    }
    else {
        Stop();
    }
    started = !started;
}

void MainWindow::Start() {
    CreateBuffers();

    left_limit = 0, right_limit = max_time_visible;

    // Reiniciar filtro
    SetupFilter();
    ResetFilters();

    // Iniciar serial e hilos
    do_serial_work = true;
    do_analysis_work = true;
    analysis_thread = std::thread(&MainWindow::AnalysisWorker, this);

    serial.open(settings->port, settings->baud_rate);
    serial_thread = std::thread(&MainWindow::SerialWorker, this);
    start_time = clock::now();
}

void MainWindow::Stop() {
    do_serial_work = false;
    do_analysis_work = false;
    analysis_cv.notify_one();

    if (serial_thread.joinable())
        serial_thread.join();
    if (analysis_thread.joinable())
        analysis_thread.join();
    serial.close();
}

void MainWindow::SelectFilter(Filter filter) {
    selected_filter = filter;
    switch (selected_filter)
    {
        case Filter::LowPass:
            min_cutoff_frequency = 1;
            max_cutoff_frequency = settings->sampling_rate / 4;
            break;
        case Filter::HighPass:
            min_cutoff_frequency = settings->sampling_rate / 4;
            max_cutoff_frequency = settings->sampling_rate / 2 - 1;
            break;
        case Filter::None:
            break;
    }
}

void MainWindow::SetupFilter() {
    switch (selected_filter)
    {
        case Filter::LowPass:
            lowpass_filter.setup(settings->sampling_rate, cutoff_frequency[1]);
            break;
        case Filter::HighPass:
            highpass_filter.setup(settings->sampling_rate, cutoff_frequency[2]);
            break;
        case Filter::None:
            break;
    }
}

void MainWindow::ResetFilters() {
    lowpass_filter.reset();
    highpass_filter.reset();
}

void MainWindow::SerialWorker() {
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
                case Filter::None:
                    break;
            }

            filter_scrollY->push(resultado);
            next_time += 1.0 / settings->sampling_rate;

            write_buffer[i] = 255 - InverseTransformSample(resultado);
        }

        serial.write(write_buffer.data(), read);
        size = scrollX->count();
    }
}

void MainWindow::AnalysisWorker() {
    while (do_analysis_work) {
        std::unique_lock lock(analysis_mutex);
        analysis_cv.wait(lock);

        if (!fft || !scrollY)
            continue;

        uint32_t available = scrollY->count();
        uint32_t max = settings->sampling_rate;
        uint32_t count = available > max ? max : available;

        auto end = scrollY->data() + available;
        fft->SetData(end - count, count);
        fft->Compute();

        std::this_thread::sleep_for(100ms);
    }
}

void MainWindow::Draw()
{
    static double elapsed_time = 0;

    int draw_size = size / settings->stride;
    if (started) {
        elapsed_time = scrollX->back();

        if (elapsed_time > max_time_visible) {
            right_limit = elapsed_time;
            left_limit = elapsed_time - max_time_visible;
        }
    }

    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize(ImVec2(width, height - statusbar_height));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::Begin("Ventana principal", &open,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar();

    auto win_pos = ImGui::GetWindowPos();
    auto win_size = ImGui::GetWindowSize();

    if (ImGui::BeginMenuBar())
    {
        MenuPuertos(settings->port);

        if (ImGui::Button("Configuración"))
            settingsWindow->Toggle();

        if (Button(started ? "Desconectar" : "Conectar", settings->port.empty())) {
            ToggleConnection();
        }
        if (settings->port.empty()) {
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

        ImPlot::PlotLine("", scrollX->data(), scrollY->data(), draw_size, 0, 0, settings->byte_stride);
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

            ImPlot::PlotLine("", scrollX->data(), filter_scrollY->data(), draw_size, 0, 0, settings->byte_stride);
            ImPlot::EndPlot();
        }

        const char* nombres[] = { "Ninguno", "Pasa bajos", "Pasa altos" };
        for (int i = 0; i < std::size(nombres); i++)
        {
            if (i > 0)
                ImGui::SameLine();

            if (i == (int)selected_filter) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
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
            && ImGui::SliderInt("Frecuencia de corte", &cutoff_frequency[(int)selected_filter], min_cutoff_frequency, max_cutoff_frequency)) {
            SetupFilter();
            ResetFilters();
        }
    }

    if (ImGui::CollapsingHeader("Análisis")) {
        analysis_cv.notify_one();
        if (ImPlot::BeginPlot("Espectro", { -1, 0 }, ImPlotFlags_NoLegend)) {
            ImPlot::SetupAxisFormat(ImAxis_Y1, MetricFormatter, (void*)"V");
            ImPlot::SetupAxisFormat(ImAxis_X1, MetricFormatter, (void*)"Hz");
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.99, settings->samples, ImGuiCond_FirstUseEver);
            ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_AutoFit);

            ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
            ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0, INFINITY);

            fft->Plot(settings->sampling_rate);
            ImPlot::EndPlot();

            // Mostrar información adicional si hay datos
            if (scrollY && scrollY->count() > 0) {
                ImGui::Text("Frecuencia: %s\tDesplazamiento %s",
                            MetricFormatter(fft->Frequency(settings->sampling_rate), "Hz").data(),
                            MetricFormatter(fft->Offset(), "V").data()
                );
            }
        }
    }

    // Barra inferior
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::SetNextWindowPos({0, win_size.y - statusbar_height});
    if (ImGui::BeginViewportSideBar("Status", 0, ImGuiDir_Down, statusbar_height, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Tiempo transcurrido: %.1fs", elapsed_time);
        if (settings->show_frame_time) {
            std::string info = std::format("Rendimiento: {:.1f} ms/frame ({:.1f} FPS)", 1000.0f / io.Framerate, io.Framerate);
            auto info_size = ImGui::CalcTextSize(info.c_str());

            ImGui::SameLine(ImGui::GetContentRegionMax().x - info_size.x);
            ImGui::Text(info.c_str());
        }
        ImGui::EndChild();
    }
    ImGui::PopStyleVar();

    ImGui::End();
}

void MainWindow::SetSize(int width, int height) {
    this->width = width;
    this->height = height;
}
