#include <mm/memory.hpp>

bool sseEnabled = false;

void memset(void *start, uint8_t value, uint64_t num) {
	if(sseEnabled) {
		int d0, d1;
	        __asm__ __volatile__(
		        "rep\n\t"
	                "stosb"
		        : "=&c" (d0), "=&D" (d1)
			:"a" (value),"1" (start),"0" (num)
	                :"memory");
	} else {
	        for (uint64_t i = 0; i < num; i++) {
		        *(uint8_t*)((uint64_t)start + i) = value;
	        }
	}
}
