#include "heap.hpp"
heap_header *firstHeader;

void init_heap(void *addr, usize size) {
	heap_header *hdr = (heap_header*)addr;
	hdr->previous = hdr->next = NULL;
	hdr->used = false;
	hdr->size = size - sizeof(heap_header);
	firstHeader = hdr;
	
	mkmi_log("Heap at 0x%x of size %d bytes\r\n", addr, size);
}

void dump_hdrs() {
	mkmi_log("HEAP_START\r\n");
	for (heap_header *hdr = firstHeader; hdr != NULL; hdr = hdr->next) {
		if (!hdr) {
			break;
		}

		mkmi_log("HEAP_HDR 0x%x, %d bytes, %s\r\n", (void*)hdr, hdr->size, hdr->used ? "used" : "unused");
	}
	mkmi_log("HEAP_END\r\n");
}

void *malloc(usize size) {
	for (heap_header *hdr = firstHeader; hdr != NULL; hdr = hdr->next) {
		if (!hdr) break;
		if (hdr->used) continue;
		if (hdr->size < size) continue;
		
		if (hdr->size == size) {
			hdr->used = true;
			return (void*)((uptr)hdr + sizeof(heap_header));
		} else if (hdr->size > size) {
			if (hdr->size > size + sizeof(heap_header)) {
				heap_header *next = (heap_header*)((uptr)hdr + sizeof(heap_header) + size);
				next->used = false;
				next->size = hdr->size - size - sizeof(heap_header);
				hdr->size -= next->size + sizeof(heap_header);

				next->previous = hdr;
				next->next = hdr->next;

				if (hdr->next) {
				    hdr->next->previous = next;
				}

				hdr->next = next;

				hdr->used = true;
				return (void*)((uptr)hdr + sizeof(heap_header));
			} else {
				hdr->used = true;
				return (void*)((uptr)hdr + sizeof(heap_header));
			}
		} else {
			continue;
		}
	}

	return NULL;
}

void free(void *ptr) {
	heap_header *hdr = (heap_header*)((uptr)ptr - sizeof(heap_header));

	hdr->used = false;

	bool cleanup = false;
	while (true) {
		if (hdr->next && !hdr->next->used) {
			cleanup = true;

			heap_header *nextHdr = hdr->next;
			if (nextHdr->next) {
				nextHdr->previous = hdr;
			}

			hdr->size += nextHdr->size + sizeof(heap_header);
			hdr->next = nextHdr->next;
		}

		if (hdr->previous && !hdr->previous->used) {
			cleanup = true;

			heap_header *prevHdr = hdr->previous;

			if(hdr->next) {
				hdr->next->previous = prevHdr;
			}

			prevHdr->next = hdr->next;
			prevHdr->size += hdr->size + sizeof(heap_header);

			hdr = prevHdr;
		}

		if (cleanup) {
			cleanup = false;
		} else {
			break;
		}
	}
}
