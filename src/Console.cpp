//
// Created by USUARIO on 2/12/2023.
//

#include <iostream>
#include "Console.h"

Console::Console() {
    console_window = GetConsoleWindow();

    if (console_window){
        WINDOWPLACEMENT placement;
        GetWindowPlacement(console_window, &placement);
        state = placement.showCmd;
    }
}

Console::~Console() {
    if (_restore){
        ShowWindow(console_window, state);
    }
}

void Console::Hide(bool persist) {
    if (!console_window)
        return;

    ShowWindow(console_window, SW_HIDE);
    if (persist)
        _restore = false;
}

void Console::Show(bool persist) {
    if (!console_window)
        return;

    ShowWindow(console_window, SW_SHOW);
    if (persist)
        _restore = false;
}

bool Console::IsOwn() {
    DWORD processId;
    GetWindowThreadProcessId(console_window, &processId);

    return GetCurrentProcessId() == processId;
}
