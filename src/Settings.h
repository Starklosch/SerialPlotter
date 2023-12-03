#pragma once
#include <string>

struct Settings {
    int sampling_rate = 3840;
    int baud_rate = sampling_rate * 10;
    int samples = sampling_rate;
    std::string port;

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
    explicit SettingsWindow(Settings& settings);

    void Toggle();
    void Draw();
};