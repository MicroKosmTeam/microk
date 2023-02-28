#pragma once
#include <stdint.h>
#include <stddef.h>

struct HeapSegHeader{
        size_t length;
        HeapSegHeader *next;
        HeapSegHeader *last;
        bool free;

        void CombineForward();
        void CombineBackward();
        HeapSegHeader *Split(size_t splitLenght);
};

namespace HEAP {
	uint64_t GetFree();
	uint64_t GetTotal();

	bool IsHeapActive();

	void InitializeHeap(void *heapAddress, size_t pageCount);
	void ExpandHeap(size_t lenght);

	void *Malloc(size_t size);
	void Free(void *address);
}
