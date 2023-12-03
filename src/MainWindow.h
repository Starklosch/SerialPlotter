#pragma once
#include <chrono>
#include <thread>

#include "Buffers.h"
#include "Serial.h"
#include "FFT.h"
#include "Settings.h"


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

    float max_time_visible = 5;

    time_point start_time = clock::now();

    int max = 0;

    // L�mites del gráfico de entrada y filtrado
    double left_limit = 0, right_limit = max_time_visible;
    double down_limit = -7, up_limit = 7;

    double next_time = 0;

    // Cantidad de puntos a dibujar
    int size = 0;

    std::vector<uint8_t> read_buffer, write_buffer;

    Settings* settings;
    SettingsWindow* settingsWindow;

    int min_cutoff_frequency = 1, max_cutoff_frequency = 100;
    int cutoff_frequency[3] = { 0, 20, 100 };
    Filter selected_filter = Filter::LowPass;

    int width, height;

public:
    MainWindow(int width, int height, Settings& config, SettingsWindow& ventanaConfig);
    ~MainWindow();

private:
    void CreateBuffers();
    void DestroyBuffers();

    double TransformSample(uint8_t v);
    uint8_t InverseTransformSample(double v);

    bool started = false;
    void ToggleConnection();

    void Start();
    void Stop();

    void SelectFilter(Filter filter);
    void SetupFilter();
    static void ResetFilters();

    bool do_serial_work = true;
    bool filter_open = false;
    void SerialWorker();

    bool do_analysis_work = true;
    void AnalysisWorker();

    float statusbar_height = 30;

public:
    bool open = true;
    void Draw();

    void SetSize(int width, int height);
};
