#include "Serial.h"
#include <format>

void printErrorMessage(DWORD error) {
	static WCHAR messageBuffer[128];

	//SetConsoleOutputCP(CP_UTF8);
	if (error == -1)
		error = GetLastError();
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

	//Ask Win32 to give us the string version of that message ID.
	//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
	size_t size = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), messageBuffer, 128, NULL);

	//return messageBuffer;
	//std::cout << "Error: ";
	WriteConsoleW(out, messageBuffer, size, 0, 0);
	//std::cout.write(messageBuffer, size);
	//std::cout << '\n';
}

Serial::~Serial() {
	close();
}

bool Serial::open(const char * port, int baud) {
	auto access = GENERIC_READ | GENERIC_WRITE;
	auto mode = OPEN_EXISTING;
	//auto share = FILE_SHARE_READ | FILE_SHARE_WRITE;
	auto flags = 0; // async FILE_FLAG_OVERLAPPED
	//auto flags = FILE_FLAG_OVERLAPPED;
	file = CreateFileA(port, access, 0, 0, mode, flags, 0);

	if (file == INVALID_HANDLE_VALUE) {
		printErrorMessage();
		return false;
	}

	//EscapeCommFunction(file, SETDTR);
	//EscapeCommFunction(file, SETRTS);

	SetupComm(file, 1024, 1024);
	PurgeComm(file, PURGE_RXCLEAR | PURGE_TXCLEAR);
	ClearCommError(file, nullptr, nullptr);

	DCB state;
	SecureZeroMemory(&state, sizeof(DCB));
	state.DCBlength = sizeof(DCB);
	GetCommState(file, &state);

	state.ByteSize = 8;
	state.BaudRate = baud;
	state.Parity = NOPARITY;
	state.StopBits = ONESTOPBIT;

	// Si están activos el Arduino se reinicia y tarda más
	state.fDtrControl = DTR_CONTROL_DISABLE;
	state.fRtsControl = RTS_CONTROL_DISABLE;


	SetCommState(file, &state);
	//EscapeCommFunction(file, SETDTR);

	COMMTIMEOUTS timeouts;
	GetCommTimeouts(file, &timeouts);
	timeouts.ReadIntervalTimeout = 0;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 1;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 1;
	SetCommTimeouts(file, &timeouts);
	return true;
}

int Serial::read(uint8_t* buffer, int size) {
	bool succeded = false;
	DWORD bytesRead = 0;

	//read_overlapped.hEvent = CreateEventA(nullptr, true, false, nullptr);
	//if (!read_overlapped.hEvent) {
	//	std::cout << "Event error";
	//	return 0;
	//}

	//if (GetLastError() != ERROR_IO_INCOMPLETE) {
	succeded = ReadFile(file, buffer, size, &bytesRead, 0);
	//}


	//bool success = GetOverlappedResult(file, &read_overlapped, &bytesRead, false);
	//if (GetLastError() != ERROR_IO_INCOMPLETE && success) {
	//	read_overlapped.Offset += bytesRead;
	//}

	//WaitForSingleObject(read_overlapped.hEvent, 100);
	//CloseHandle(read_overlapped.hEvent);

	return bytesRead;
}

int Serial::write(uint8_t* buffer, int size) {
	bool succeded = false;
	DWORD bytesWritten = 0;

	succeded = WriteFile(file, buffer, size, &bytesWritten, 0);
	return bytesWritten;
}

int Serial::read_overlapped(uint8_t* buffer, int size) {
	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (overlapped.hEvent == NULL) {
		std::cout << "Couldn't create event\n";
		return 0;
	}

	DWORD bytesRead = 0;
	bool result = ReadFile(file, buffer, size, &bytesRead, &overlapped);

	if (result) {
		CloseHandle(overlapped.hEvent);
		return bytesRead;
	}

	if (GetLastError() != ERROR_IO_PENDING) {
		std::cout << "Unexpected error: ";
		printErrorMessage();
		std::cout << '\n';
		return 0;
	}

	switch (WaitForSingleObject(file, 1000)) {
	case WAIT_OBJECT_0:
		std::cout << "Signaled\n";
		break;
	case WAIT_TIMEOUT:
		std::cout << "Timeout\n";
		CloseHandle(overlapped.hEvent);
		return 0;
	}

	if (!GetOverlappedResult(file, &overlapped, &bytesRead, false)) {
		std::cout << "Unexpected error: ";
		printErrorMessage();
		std::cout << '\n';
	}
	CloseHandle(overlapped.hEvent);
	return bytesRead;
}

int Serial::write_overlapped(uint8_t* buffer, int size) {
	return 0;
}

void Serial::close() {
	CloseHandle(file);
}

size_t Serial::available()
{
	COMSTAT stat;
	ClearCommError(file, nullptr, &stat);
	return stat.cbInQue;
}
