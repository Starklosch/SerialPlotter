#pragma once
#include "Serial.h"
#include "Buffers.h"
#include "stdint.h"

class Monitor {
public:
	Monitor(size_t input_capacity, size_t output_capacity, size_t buffer_size);

	~Monitor();

	void start(const char* port, uint32_t baud);

	void stop();

	size_t available();

	size_t pending();

	template <typename T>
	T get() {
		T value;
		if (input_queue.size() >= sizeof(T))
			input_queue.read(&value, sizeof(T));
		return value;
	}

	template <typename T>
	bool put(const T value) {
		if (output_queue.available() < sizeof(T))
			return false;

		output_queue.write(&value, sizeof(T));
		return true;
	}

	size_t read(uint8_t* buffer, int count);

	size_t write(const uint8_t* buffer, int count);

	std::function<void(int read)> read_callback;

private:
	void read_from_serial();
	void write_from_serial();

	bool do_work = false;
	Serial serial;
	std::thread input_thread, output_thread;

	int buffer_size;
	uint8_t* input_buffer, * output_buffer;
	Buffer<uint8_t> input_queue, output_queue;
};