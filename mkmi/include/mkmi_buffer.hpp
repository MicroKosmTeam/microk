/*
   File: include/mkmi/mkmi_buffer.hpp

   MKMI buffer types and managment
*/

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

namespace MKMI {
namespace BUFFER {
	enum BufferType {
		COMMUNICATION_USERKERNEL = 0,
		COMMUNICATION_INTERKERNEL = 1,
		COMMUNICATION_INTERUSER = 2,
	};

	enum BufferOperation {
		OPERATION_READDATA = 0,
		OPERATION_WRITEDATA = 1,
	};

	struct Buffer {
		uintptr_t address;
		bool readable;
		BufferType type;
		size_t size;
	} __attribute__((packed));

	Buffer *Create(BufferType type, size_t size);
	uint64_t IOCtl(Buffer *buffer, BufferOperation operation, ...);
	uint64_t Delete(Buffer *buffer);
}
}
