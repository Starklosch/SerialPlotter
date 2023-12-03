#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>

void printErrorMessage(uint32_t error = -1);

std::vector<std::string> EnumerateComPorts();

class Serial {
    HANDLE file = nullptr;

public:
    ~Serial();

    bool open(std::string port, int baud = 9600);

    int read(uint8_t* buffer, int size);

    int write(uint8_t* buffer, int size);

    void close();

    size_t available();
};