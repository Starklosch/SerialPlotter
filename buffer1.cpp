template <typename T>
class Buffer {
public:
	Buffer(int capacity) : capacity(capacity + 1) {
		data = new T[capacity + 1];
	}

	Buffer(const Buffer& other) {
		capacity = other.capacity;
		data = new T[capacity];
		for (size_t i = 0; i < capacity; i++)
			data[i] = other.data[i];
	}

	Buffer(Buffer&& other) {
		capacity = other.capacity;
		data = other.data;
		other.data = nullptr;
	}

	Buffer& operator=(const Buffer& other) {
		if (capacity < other.capacity)
			delete[] data;

		capacity = other.capacity;
		data = new T[capacity];
		for (size_t i = 0; i < capacity; i++)
			data[i] = other.data[i];
	}

	Buffer& operator=(Buffer&& other) {
		delete[] data;
		capacity = other.capacity;
		data = other.data;
		other.data = nullptr;
	}

	~Buffer() {
		delete[] data;
	}

	int size() const {
		int count = end - start;
		return count >= 0 ? count : count + capacity;
	}

	int available() const {
		return capacity - size() - 1;
	}

	int write(const T* buffer, int count) {
		int free = available();
		if (free == 0)
			return 0;

		if (count > free)
			count = free;

		int right_free = capacity - end;
		int right_count = count < right_free ? count : right_free;
		int left_count = count - right_count;

		std::copy(buffer, &buffer[right_count], &data[end]);
		if (left_count > 0)
			std::copy(&buffer[right_count], &buffer[count], data);

		end = (end + count) % capacity;
		return count;
	}

	int read(T* buffer, int count) {
		int length = size();
		if (length == 0)
			return 0;

		if (count > length)
			count = length;

		int right_available = capacity - start;
		int right_count = count < right_available ? count : right_available;
		int left_count = count - right_count;

		std::copy(&data[start], &data[start + right_count], buffer);
		if (left_count > 0)
			std::copy(data, &data[left_count], &buffer[right_count]);

		start = (start + count) % capacity;
		return count;
	}

	void clear() {
		start = end = 0;
	}

	void skip(int count) {
		int length = size();
		if (count > length)
			count = length;
		start = (start + count) % capacity;
	}

	T& operator[](int i) {
		if (i < 0 || i >= size())
			throw std::exception("Out of index");

		int real_pos = (i + start) % capacity;
		return data[real_pos];
	}

	void print() const {
		for (size_t i = 0; i < capacity; i++)
			std::cout << data[i] << ", ";
		std::cout << "\b\b   \n";
		for (size_t i = 0; i < size(); i++)
			std::cout << data[(i + start) % capacity] << ", ";
		std::cout << "\b\b   \n";
	}

private:

	int capacity;
	int start = 0, end = 0;
	T* data;
};