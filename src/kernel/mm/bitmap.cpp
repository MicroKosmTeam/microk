#include <mm/bitmap.h>

bool Bitmap::operator[](uint64_t index) {
        uint64_t byte_index = index / 8;
        uint8_t bit_index = index % 8;
        uint8_t bit_indexer = 0b10000000 >> bit_index;

        if ((buffer[byte_index] & bit_indexer) > 0) {
                return true;
        }

        return false;
}

void Bitmap::set(uint64_t index, bool value) {
        uint64_t byte_index = index / 8;
        uint8_t bit_index = index % 8;
        uint8_t bit_indexer = 0b10000000 >> bit_index;

        buffer[byte_index] &= ~bit_indexer;
        if (value) {
                buffer[byte_index] |= bit_indexer;
        }
}
