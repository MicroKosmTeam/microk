#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
struct HeapSegHeader{
        size_t length;
        HeapSegHeader *next;
        HeapSegHeader *last;
        bool free;

        void CombineForward();
        void CombineBackward();
        HeapSegHeader *Split(size_t splitLenght);
};

void InitializeHeap(void *heapAddress, size_t pageCount);
void VisualizeHeap();
void ExpandHeap(size_t lenght);

extern "C" {
#endif
void *malloc(size_t size);
void free(void *address);
#ifdef __cplusplus
}
inline void *operator new(size_t size) { return malloc(size); }
inline void *operator new[](size_t size) { return malloc(size); }
inline void operator delete(void *p) { free(p); }
inline void operator delete(void *p, size_t size) { free(p); }
inline void operator delete[](void *p) { free(p); }
#endif

