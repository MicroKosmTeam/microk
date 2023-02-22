#include <mm/heap.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <sys/printk.h>

#define PREFIX "[KHEAP] "

void *heapStart;
void *heapEnd;
HeapSegHeader *lastHeader;

void HeapSegHeader::CombineForward() {
        if(next == NULL) return;
        if(!next->free) return;

        if(next == lastHeader) lastHeader = this;

        if(next->next != NULL) {
                next->next->last = this;
        }

        length = length + next->length + sizeof(HeapSegHeader);

        next = next->next;
}

void HeapSegHeader::CombineBackward() {
        if (last != NULL && last->free) last->CombineForward();
}

HeapSegHeader *HeapSegHeader::Split(size_t splitlength) {
        if (splitlength < 0x10) return NULL;
        int64_t splitSeglength = length - splitlength - (sizeof(HeapSegHeader));
        if (splitSeglength < 0x10) return NULL;

        HeapSegHeader *newSplitHeader = (HeapSegHeader*)((size_t)this + splitlength + sizeof(HeapSegHeader));
	if(next == NULL) next = newSplitHeader;
	else next->last = newSplitHeader;            // Set the next segment's last segment to our new segment
        newSplitHeader->next = next;            // Set the new segment's next segment to our original next
        next = newSplitHeader;                  // Set our new segment to the next segment
        newSplitHeader->last = this;            // Set our new segment's last segment to us
        newSplitHeader->length = splitSeglength;// Set the new header's length
        newSplitHeader->free = free;            // Make sure both free are the same
        length = splitlength;                   // Set the original segment's length to the nes one

        if (lastHeader == this) lastHeader = newSplitHeader;
        return newSplitHeader;
}

void InitializeHeap(void *heapAddress, size_t pageCount) {
        void *pos = heapAddress;
	printk(PREFIX "Initializing the heap at 0x%x with %d pages.\n", heapAddress, pageCount);

        for (size_t i = 0; i < pageCount; i++) {
		VMM::MapMemory(pos, PMM::RequestPage());
                pos = (void*)((size_t)pos + 0x1000); // Advancing
        }

        size_t heaplength = pageCount * 0x1000;
	printk(PREFIX "%d bytes memory allocated.\n", heaplength);

        heapStart = heapAddress;
        heapEnd = (void*)((size_t)heapStart + heaplength);

        HeapSegHeader *startSeg = (HeapSegHeader*)heapAddress;
        startSeg->length = heaplength - sizeof(HeapSegHeader);
        startSeg->next = NULL;
        startSeg->last = NULL;
        startSeg->free = true;
        lastHeader = startSeg;
}

void *malloc(size_t size) {
        if (size % 0x10 > 0){ // Not multiple of 0x10
                size -= (size % 0x10);
                size += 0x10;
        }

        if (size == 0) return NULL;

        HeapSegHeader *currSeg = (HeapSegHeader*)heapStart;
        while (true) {
                if (currSeg-> free) {
                        if (currSeg->length > size) {
                                currSeg->Split(size);
                                currSeg->free = false;
                                return (void*)((uint64_t)currSeg + sizeof(HeapSegHeader));
                        } else if (currSeg->length == size) {
                                currSeg->free = false;
                                return (void*)((uint64_t)currSeg + sizeof(HeapSegHeader));
                        }
                }

                if (currSeg->next == NULL) break;
                currSeg = currSeg->next;
        }

        ExpandHeap(size);
        return malloc(size);
}

void free(void *address) {
        HeapSegHeader *segment = (HeapSegHeader*)address - 1;
        segment->free = true;
        segment->CombineForward();
        segment->CombineBackward();
}

void ExpandHeap(size_t length) {
        if (length % 0x1000) { // We can't allocate less that a page
                length -= length % 0x1000;
                length += 0x1000;
        }

        size_t pageCount = length / 0x1000;
        HeapSegHeader *newSegment = (HeapSegHeader*)heapEnd;

        for (size_t i = 0; i < pageCount; i++) {
		VMM::MapMemory(heapEnd, PMM::RequestPage());
                heapEnd = (void*)((size_t)heapEnd + 0x1000);
        }

        newSegment->free = true;
        newSegment->last = lastHeader;
        lastHeader->next = newSegment;
        lastHeader = newSegment;
        newSegment->next = NULL;
        newSegment->length = length - sizeof(HeapSegHeader);
        newSegment->CombineBackward();

	printk(PREFIX "%d bytes memory allocated.\n", length);
}
