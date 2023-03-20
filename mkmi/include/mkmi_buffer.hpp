/*
   File: include/mkmi/mkmi_buffer.hpp

   MKMI buffer types and managment
*/

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <mkmi_module.hpp>

namespace MKMI {
namespace BUFFER {
	enum BufferType {
		COMMUNICATION_USERKERNEL   = 0xC00,
		COMMUNICATION_INTERKERNEL  = 0xC01,
		COMMUNICATION_INTERUSER    = 0xC02,

		DATA_USER_GENERIC          = 0xD00,
		DATA_KERNEL_GENERIC        = 0xD01,
	};

	enum BufferOperation {
		OPERATION_READDATA         = 0x00,
		OPERATION_WRITEDATA        = 0x01,
	};

	struct Buffer {
		uintptr_t address;
		bool readable;
		BufferType type;
		size_t size;
	}__attribute__((packed));

	Buffer *Create(BufferType type, size_t size);
	uint64_t IOCtl(Buffer *buffer, BufferOperation operation, ...);
	uint64_t Delete(Buffer *buffer);
}
}
