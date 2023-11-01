#pragma once

#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>

void printErrorMessage(DWORD error = -1);

std::vector<std::string> EnumerateComPorts();

class Serial {
	HANDLE file = 0;
	//OVERLAPPED write_overlapped = {};
	//OVERLAPPED read_overlapped = {};

public:
	~Serial();

	bool open(const char* port, int baud = 9600);

	int read(uint8_t* buffer, int size);

	int write(uint8_t* buffer, int size);

	int read_overlapped(uint8_t* buffer, int size);
	int write_overlapped(uint8_t* buffer, int size);

	void close();

	size_t available();
};