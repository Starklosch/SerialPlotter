#include <chrono>
#include <thread>
#include "Monitor.h"

using namespace std::chrono_literals;

void reception_test() {
	using clock = std::chrono::high_resolution_clock;
	using time_point = clock::time_point;
	using duration = std::chrono::duration<double>;
	time_point start = clock::now(), now;
	double dur = 0;
	int read = 0;

	Serial ser;
	ser.open("COM5", 38400);
	uint8_t buff[1024];
	while (dur <= 1) {
		now = clock::now();
		dur = duration(now - start).count();
		read += ser.read(buff, 1024);
	}
	std::cout << std::format("{} bytes read in {} seconds.\n", read, dur);
}

void monitor_test() {
	using clock = std::chrono::high_resolution_clock;
	using time_point = clock::time_point;
	using duration = std::chrono::duration<double>;

	std::vector<uint8_t> buff(4 * 1024);
	Monitor monitor(16 * 1024, 2 * 1024, 1024);
	int read = 0;

	monitor.start("COM5", 38400);

	std::this_thread::sleep_for(1s);

	int available = monitor.available();

	time_point start = clock::now(), now;
	read = monitor.read(buff.data(), available);
	now = clock::now();

	std::cout << std::format("{} bytes available\n", available);
	std::cout << std::format("{} bytes read in {} seconds.\n", read, duration(now - start).count());
	std::cout << std::format("{} bytes still available\n", monitor.available());
}

void interval() {
	using clock = std::chrono::high_resolution_clock;
	using time_point = clock::time_point;
	using duration = std::chrono::duration<double>;

	time_point start = clock::now();
	for (size_t i = 0; i < 10; i++)
	{
		time_point next = start + 1s;
		std::this_thread::sleep_until(next);
		std::cout << "Now: " << duration(clock::now() - start).count() << '\n';
		start = next;
	}
}

void write_test() {
	using clock = std::chrono::high_resolution_clock;
	using time_point = clock::time_point;
	using duration = std::chrono::duration<double>;

	double f = 4, pi = 3.141592653589793;

	Monitor monitor(16 * 1024, 4 * 1024, 1024);
	monitor.start("COM5", 38400);

	time_point start = clock::now(), end = start + 1s;

	while (clock::now() < end) {
		double t = duration(clock::now() - start).count();
		double v = 255 / 2.f * (sin(2 * pi * f * t) + 1);
		monitor.put((uint8_t)v);
	}
}

void read_test() {
	using clock = std::chrono::high_resolution_clock;
	using time_point = clock::time_point;
	using duration = std::chrono::duration<double>;

	uint8_t buffer[1024];

	Monitor monitor(16 * 1024, 4 * 1024, 1024);
	monitor.read_callback = [&](int) {
		int n = monitor.read(buffer, 1024);
		//std::cout.write((char*)buffer, n);
		for (size_t i = 0; i < n; i++)
			std::cout << +buffer[i] << '\n';
	};

	monitor.start("COM5", 115200);
	//Serial serial;
	//serial.open("COM5", 115200);

	time_point start = clock::now(), end = start + 2s;

	while (clock::now() < end) {
		//int n = serial.read(buffer, 1024);
	}
}