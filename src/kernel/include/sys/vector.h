#pragma once
#include <stdint.h>
#include <mm/heap.h>

template <typename Type>
class Vector{
public:
	Vector() {
		array = new Type[1];
		capacity = 1;
		currentElement = 0;
	}

	~Vector() {
		delete[] array;
	}

	void Push(Type data) {
		if (currentElement == capacity) {
			Type *temp = new Type[2 * capacity];

			for (int i = 0; i < capacity; i++) {
				temp[i] = array[i];
			}

			delete[] array;
			capacity *= 2;
			array = temp;
		}

		array[currentElement] = data;
		currentElement++;
	}

	void Push(Type data, uint64_t index) {
		if (index == capacity) Push(data);
		else array[index] = data;
	}

	Type Get(uint64_t index) {
		if (index < currentElement) return array[index];
	}

	Type Pop() {
		currentElement--;
		return array[currentElement+1];
	}

	Type operator[](uint64_t index) {
		Get(index);
	}

	uint64_t GetCurrent() { return currentElement; }

	uint64_t GetCapacity() { return capacity; }
private:
	Type *array;
	uint64_t capacity;
	uint64_t currentElement;
};

