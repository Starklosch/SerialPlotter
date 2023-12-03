#include "Serial.h"
#include <format>
#include <algorithm>

// Parte del código tomado de https://stackoverflow.com/a/17387176
void printErrorMessage(uint32_t error) {
    static WCHAR messageBuffer[128];

    if (error == -1)
        error = GetLastError();
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    size_t size = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), messageBuffer, 128, nullptr);

    WriteConsoleW(out, messageBuffer, size, nullptr, nullptr);
}

std::vector<std::string> EnumerateComPorts() {
    std::vector<std::string> com_ports;

    HKEY key;
    LSTATUS result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_QUERY_VALUE, &key);
    if (result != ERROR_SUCCESS)
        return com_ports;

    for (DWORD i = 0; result != ERROR_NO_MORE_ITEMS; i++)
    {
        char name[64];
        uint8_t data[64];
        DWORD nameChars = 64, dataSize = 64;
        DWORD type;
        result = RegEnumValueA(key, i, name, &nameChars, nullptr, &type, data, &dataSize);

        if (result == ERROR_SUCCESS)
            com_ports.emplace_back((char*)data);
    }
    RegCloseKey(key);

    std::sort(com_ports.begin(), com_ports.end());
    return com_ports;
}

Serial::~Serial() {
    close();
}

bool Serial::open(std::string port, int baud) {
    DWORD access = GENERIC_READ | GENERIC_WRITE;
    DWORD mode = OPEN_EXISTING;

    const std::string prefijo = "\\\\.\\";
    if (!port.starts_with(prefijo))
        port.insert(0, prefijo);
    
    file = CreateFileA(port.c_str(), access, 0, 0, mode, 0, 0);

    if (file == INVALID_HANDLE_VALUE) {
        printErrorMessage();
        return false;
    }

    SetupComm(file, 2048, 2048);
    PurgeComm(file, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR);
    ClearCommError(file, nullptr, nullptr);

    DCB state { .DCBlength = sizeof(DCB) };
    GetCommState(file, &state);

    // Configurar comunicaci�n
    state.ByteSize = 8;
    state.BaudRate = baud;
    state.Parity = NOPARITY;
    state.StopBits = ONESTOPBIT;

    // Si est�n activos el Arduino se reinicia y tarda más
    state.fDtrControl = DTR_CONTROL_DISABLE;
    state.fRtsControl = RTS_CONTROL_DISABLE;
    SetCommState(file, &state);

    COMMTIMEOUTS timeouts;
    GetCommTimeouts(file, &timeouts);
    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 1000;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 1000;
    SetCommTimeouts(file, &timeouts);
    return true;
}

int Serial::read(uint8_t* buffer, int size) {
    DWORD bytesRead = 0;

    ReadFile(file, buffer, size, &bytesRead, nullptr);
    return bytesRead;
}

int Serial::write(uint8_t* buffer, int size) {
    DWORD bytesWritten = 0;

    WriteFile(file, buffer, size, &bytesWritten, nullptr);
    return bytesWritten;
}

void Serial::close() {
    CloseHandle(file);
    file = nullptr;
}

size_t Serial::available()
{
    COMSTAT stat;
    ClearCommError(file, nullptr, &stat);
    return stat.cbInQue;
}
