#include <imgui.h>

#include "Serial.h"
#include "Settings.h"

using namespace std::string_literals;

const int bauds[] = { 1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200, 230400, 250000, 460800, 500000, 921600, 1000000, 2000000 };
const int frecuencias[] = { 120, 240, 480, 960, 1440, 1920, 3840, 5760, 11520, 23040, 25000, 46080, 50000, 92160, 100000, 2000000 };

#include "Widgets.h"

void ComboFrecuenciaMuestreo(int& selected) {
    std::function to_string = [](int n) { return std::to_string(n); };

    combo("Frecuencia", selected, frecuencias, to_string);
}

void ComboBaudRate(int& selected) {
    std::function to_string = [](int n) { return std::to_string(n); };

    combo("Velocidad", selected, bauds, to_string);
}

void ComboPuertos(std::string& selected_port) {
    auto puertos = EnumerateComPorts();
    std::function to_string = [](std::string s) { return s; };

    combo("Puerto", selected_port, puertos, to_string, "No hay ningún dispositivo conectado");
}

SettingsWindow::SettingsWindow(Settings& settings) : settings(settings), open(settings.open)
{
}

void SettingsWindow::Toggle() {
    open = !open;
}

void SettingsWindow::Draw() {
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
            settings.stride = (int)exp2(stride_exp);
            settings.byte_stride = sizeof(double) * settings.stride;
        }
        SetItemTooltip("Dibuja 1 de cada 2^n muestras para mejorar el rendimiento.");

        Checkbox("Mostrar tiempo de renderizado", &settings.show_frame_time);
        TreePop();
    }

    End();
}