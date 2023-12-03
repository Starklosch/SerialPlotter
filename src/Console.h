#pragma once

#include <Windows.h>

class Console {
    HWND console_window;
    int state;
    bool _restore = true;

public:
    explicit Console();

    ~Console();

    void Hide(bool persist = false);

    void Show(bool persist = false);

    bool IsOwn();
};
