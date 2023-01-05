#include <mm/bitmap.h>

bool Bitmap::operator[](uint64_t index) {
        return get(index);
}

bool Bitmap::get(uint64_t index) {
        if (index > size * 8) return false;
        uint64_t byte_index = index / 8;
        uint8_t bit_index = index % 8;
        uint8_t bit_indexer = 0b10000000 >> bit_index;

        if ((buffer[byte_index] & bit_indexer) > 0) {
                return true;
        }

        return false;
}

bool Bitmap::set(uint64_t index, bool value) {
        if (index > size * 8) return false;
        uint64_t byte_index = index / 8;
        uint8_t bit_index = index % 8;
        uint8_t bit_indexer = 0b10000000 >> bit_index;

        buffer[byte_index] &= ~bit_indexer;
        if (value) {
                buffer[byte_index] |= bit_indexer;
        }

        return true;
}
