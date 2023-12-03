#pragma once

#include <functional>
#include <future>
#include <format>
#include <implot.h>

template <typename T>
class ScrollBuffer {
public:
	ScrollBuffer(uint32_t capacity, uint32_t view) :
		capacity(capacity), view(view)
	{
		m_data = new T[capacity];
	}

	ScrollBuffer(const ScrollBuffer& other) {
		capacity = other.capacity;
		m_size = other.m_size;
		view = other.view;
		m_data = new T[capacity];
		uint32_t copy = view < m_size ? view : m_size;
		for (size_t i = 0; i < copy; i++)
			m_data[i] = other.m_data[i + offset];
	}

	ScrollBuffer(ScrollBuffer&& other) noexcept {
		capacity = other.capacity;
		m_size = other.m_size;
		view = other.view;
		offset = other.offset;
		m_data = other.m_data;
		other.m_data = nullptr;
	}

	ScrollBuffer& operator=(const ScrollBuffer& other) {
		if (capacity < other.capacity) {
			delete[] m_data;
			m_data = new T[other.capacity];
		}

		view = other.view;
		m_size = other.m_size;
		offset = 0;
		uint32_t copy = view < m_size ? view : m_size;
		for (size_t i = 0; i < copy; i++)
			m_data[i] = other.m_data[i + offset];

        return *this;
	}

	ScrollBuffer& operator=(ScrollBuffer&& other) {
		delete[] m_data;
		m_size = other.m_size;
		capacity = other.capacity;
		view = other.view;
		offset = other.offset;
		m_data = other.m_data;
		other.m_data = nullptr;
        return *this;
	}

	~ScrollBuffer() {
		delete[] m_data;
	}

	void write(T* buffer, uint32_t count) {
		if (count > view) {
			buffer += count - view;
			count = view;
		}

		uint32_t new_size = m_size + count;
		if (new_size <= capacity) {
			std::copy(buffer, &buffer[count], &m_data[m_size]);
			m_size = new_size;
			if (m_size > view)
				offset = new_size - view;
		}
		else if (count == view) {
			std::copy(buffer, &buffer[count], m_data);
			m_size = view;
			offset = 0;
		}
		else {
			uint32_t left_count = view - count;
			uint32_t left = capacity - left_count;
			std::copy(buffer, &buffer[count], &m_data[left_count]);
			std::copy(&m_data[left], &m_data[m_size], m_data);
			m_size = view;
			offset = 0;
		}
	}

	void clear() {
		offset = 0;
		m_size = 0;
	}

	void push(T value) {
		if (m_size == capacity) {
			uint32_t count = view - 1;
			uint32_t start = capacity - count;
			std::copy(&m_data[start], &m_data[capacity], m_data);
			m_data[count] = value;
			m_size = view;
			offset = 0;
			return;
		}

		m_data[m_size++] = value;
		if (m_size > view)
			offset = m_size - view;
	}

	uint32_t count() const {
		return view < m_size ? view : m_size;
	}

	uint32_t size() const {
		return m_size;
	}

	T& front() {
		return m_data[offset];
	}

	T& back() {
		return m_data[offset + count() - 1];
	}

	T& operator[](uint32_t index) {
		return m_data[(offset + index) % capacity];
	}

	const T* data() const {
		return m_data + offset;
	}

private:

	uint32_t capacity, view, m_size = 0, offset = 0;
	T* m_data;
};

template <typename T>
class Buffer {
public:
	Buffer(int capacity) : _capacity(capacity + 1) {
		data = new T[capacity + 1];
	}

	Buffer(const Buffer& other) {
		_capacity = other._capacity;
		data = new T[_capacity];
		for (size_t i = 0; i < _capacity; i++)
			data[i] = other.data[i];
	}

	Buffer(Buffer&& other) {
		_capacity = other._capacity;
		data = other.data;
		other.data = nullptr;
	}

	Buffer& operator=(const Buffer& other) {
		if (_capacity < other._capacity) {
			delete[] data;
			data = new T[other._capacity];
		}

		_capacity = other._capacity;
		for (size_t i = 0; i < _capacity; i++)
			data[i] = other.data[i];
	}

	Buffer& operator=(Buffer&& other) {
		delete[] data;
		_capacity = other._capacity;
		data = other.data;
		other.data = nullptr;
	}

	~Buffer() {
		delete[] data;
	}

	size_t capacity() const {
		return _capacity - 1;
	}

	size_t size() const {
		size_t count = end - start;
		if (end < start)
			count += _capacity;
		return count;
	}

	size_t available() const {
		return _capacity - size() - 1;
	}

	size_t write(const T* buffer, int count) {
		size_t free = available();
		if (free == 0)
			return 0;

		if (count > free)
			count = free;

		size_t right_free = _capacity - end;
		size_t right_count = count < right_free ? count : right_free;
		size_t left_count = count - right_count;

		std::lock_guard guard(data_mutex);
		std::copy(buffer, &buffer[right_count], &data[end]);
		if (left_count > 0)
			std::copy(&buffer[right_count], &buffer[count], data);

		end = (end + count) % _capacity;
		return count;
	}

	size_t read(T* buffer, int count) {
		size_t length = size();
		if (length == 0)
			return 0;

		if (count > length)
			count = length;

		size_t right_available = _capacity - start;
		size_t right_count = count < right_available ? count : right_available;
		size_t left_count = count - right_count;

		std::lock_guard guard(data_mutex);
		std::copy(&data[start], &data[start + right_count], buffer);
		if (left_count > 0)
			std::copy(data, &data[left_count], &buffer[right_count]);

		start = (start + count) % _capacity;
		return count;
	}

	void clear() {
		start = end = 0;
	}

	void skip(size_t count) {
		size_t length = size();
		if (count > length)
			count = length;
		start = (start + count) % _capacity;
	}

	T& operator[](size_t i) {
		if (i < 0 || i >= size())
			throw std::exception("Out of index");

		size_t real_pos = (i + start) % _capacity;
		return data[real_pos];
	}

	void print() const {
		for (size_t i = 0; i < _capacity; i++)
			std::cout << data[i] << ", ";
		std::cout << "\b\b   \n";
		for (size_t i = 0; i < size(); i++)
			std::cout << data[(i + start) % _capacity] << ", ";
		std::cout << "\b\b   \n";
	}

private:

	int _capacity;
	std::atomic_size_t start = 0, end = 0;
	T* data;

	std::mutex data_mutex;
};