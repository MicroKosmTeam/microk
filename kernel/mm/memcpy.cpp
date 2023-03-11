#include <mm/memory.hpp>
#include <arch/x64/cpu/cpu.hpp>

void memcpy(void *dest, void *src, size_t n) {
	if (false) {
		int i;
	        for(i=0; i<n/16; i++) {
			__asm__ __volatile__ (
		                        "movups (%0), %%xmm0\n"
	                                "movntdq %%xmm0, (%1)\n"
					::"r"(src),
				        "r"(dest) : "memory");

			src += 16;
		        dest += 16;
	        }

		if(n & 7) {
	                n = n&7;

			int d0, d1, d2;
			__asm__ __volatile__(
				"rep ; movsl\n\t"
	                        "testb $2,%b4\n\t"
		                "je 1f\n\t"
			        "movsw\n"
				"1:\ttestb $1,%b4\n\t"
	                        "je 2f\n\t"
		                "movsb\n"
			        "2:"
				: "=&c" (d0), "=&D" (d1), "=&S" (d2)
	                        :"0" (n/4), "q" (n),"1" ((long) dest),"2" ((long) src)
		                : "memory");
		}
	} else {
		char *csrc = (char *)src;
	        char *cdest = (char *)dest;

		for (int i=0; i<n; i++) cdest[i] = csrc[i];
	}
}
