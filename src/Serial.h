#pragma once

#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>

void printErrorMessage(DWORD error = -1);

std::vector<std::string> EnumerateComPorts();

class Serial {
public:
    HANDLE file = 0;
    OVERLAPPED write_overlapped = {};
    OVERLAPPED read_overlapped = {};

    ~Serial();

    bool open(std::string port, int baud = 9600);

    int read(uint8_t* buffer, int size);

    int write(uint8_t* buffer, int size);

    void close();

    size_t available();
};