#include <arch/x64/mm/paging.hpp>

void PageDirectoryEntry::SetFlag(PT_Flag flag, bool enabled) {
        uint64_t bit_selector = (uint64_t)1 << flag;
        value &= ~bit_selector;
        if (enabled) {
                value |= bit_selector;
        }
}

bool PageDirectoryEntry::GetFlag(PT_Flag flag) {
        uint64_t bit_selector = (uint64_t)1 << flag;
        return value & bit_selector > 0 ? true : false;
}

void PageDirectoryEntry::SetAddress(uint64_t address) {
        address &= 0x000000ffffffffff;
        value &= 0xfff0000000000fff;
        value |= (address << 12);
}

uint64_t PageDirectoryEntry::GetAddress() {
        return (value & 0x000ffffffffff000) >> 12;
}
