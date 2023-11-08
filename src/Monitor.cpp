#include "Monitor.h"

Monitor::Monitor(size_t input_capacity, size_t output_capacity, size_t buffer_size) :
	input_queue(input_capacity),
	output_queue(output_capacity),
	buffer_size(buffer_size)
{
	input_buffer = new uint8_t[buffer_size * 2];
	output_buffer = &input_buffer[buffer_size];
}

Monitor::Monitor(Monitor&& other) :
	buffer_size(other.buffer_size),
	input_buffer(other.input_buffer),
	output_buffer(other.output_buffer),
	input_queue(std::move(other.input_queue)),
	output_queue(std::move(other.output_queue))
{
	other.input_buffer = nullptr;
}

Monitor::~Monitor() {
	stop();
	delete[] input_buffer;
}

void Monitor::start(const char* port, uint32_t baud) {
	serial.open(port, baud);
	do_work = true;
	input_thread = std::thread(&Monitor::read_from_serial, this);
	output_thread = std::thread(&Monitor::write_from_serial, this);
}

size_t Monitor::pending() {
	return output_queue.size();
}

void Monitor::write_from_serial() {
	while (do_work) {
		int available = output_queue.size();
		if (available > 0) {
			int bytes_read = output_queue.read(output_buffer, buffer_size);
			serial.write(output_buffer, bytes_read);
		}
	}
}

void Monitor::read_from_serial() {
	while (do_work) {
		int bytes_read = serial.read(input_buffer, buffer_size);
		if (bytes_read) {
			input_queue.write(input_buffer, bytes_read);
			if (read_callback)
				std::async(std::launch::async, read_callback, bytes_read);
		}
	}
}

size_t Monitor::write(const uint8_t* buffer, int count) {
	return output_queue.write(buffer, count);
}

size_t Monitor::read(uint8_t* buffer, int count) {
	return input_queue.read(buffer, count);
}

size_t Monitor::available() {
	return input_queue.size();
}

void Monitor::stop() {
	serial.close();
	do_work = false;
	if (input_thread.joinable())
		input_thread.join();
	if (output_thread.joinable())
		output_thread.join();
}

bool Monitor::started()
{
	return do_work;
}
