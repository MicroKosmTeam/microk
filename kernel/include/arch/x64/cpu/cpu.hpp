/*
   File: include/arch/x64/cpu/cpu.hpp
*/

#pragma once

namespace x86_64 {
	/* This class is used to manage an x86 CPU */
	class x86CPU {
	public:
		x86CPU() {
			Init();
		}

		/* Function that initializes the struct */
		void Init();

		/* Declared inline for faster execution
		   as it is run in functions like memcpy */
		inline bool IsSSE() {
			return sseStatus;
		}

		/* Function that returns the CPU vendor string */
		void GetVendor(char *string);
	private:
		/* Member function that checks for SSE */
		int CheckSSE();
		/* Wether SSE is actually active */
		bool sseStatus = false;
	};
}
