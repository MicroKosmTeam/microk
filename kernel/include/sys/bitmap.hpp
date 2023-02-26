#pragma once
#include <stddef.h>
#include <stdint.h>

class Bitmap {
public:
	bool operator[](uint64_t index);
	bool Get(uint64_t index);
	bool Set(uint64_t index, bool value);
	size_t size;
	uint8_t *buffer;
};
