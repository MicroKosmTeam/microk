#include <mkmi/mkmi_buffer.hpp>
#include <mm/pmm.hpp>
#include <mm/memory.hpp>

namespace MKMI {
namespace BUFFER {
uint64_t InterkernelIOCtl(Buffer *buffer, BufferOperation operation, va_list ap) {
	uint64_t result = 0;

	switch(operation) {
		case OPERATION_READDATA: {
			uint64_t *destBuffer = va_arg(ap, uint64_t*);
			size_t destBufferLength = va_arg(ap, size_t);
			uint64_t *startBuffer = (uint64_t*)buffer->address;
			size_t readSize =  buffer->size > destBufferLength ? destBufferLength : buffer->size;
			memcpy(destBuffer, startBuffer,	readSize);
			result = 1;
			}
			break;
		case OPERATION_WRITEDATA: {
			uint64_t *startBuffer = va_arg(ap, uint64_t*);
			uint64_t *destBuffer = (uint64_t*)buffer->address;
			size_t startBufferLength = va_arg(ap, size_t);
			size_t readSize =  buffer->size > startBufferLength ? startBufferLength : buffer->size;
			memcpy(destBuffer, startBuffer,	readSize);
			result = 2;
			}
			break;
		default:
			result = 0;
			break;
	}

	return result;
}

Buffer *Create(BufferType type, size_t size) {
	Buffer buffer;
	Buffer *destBuffer;

	switch(type) {
		case COMMUNICATION_INTERKERNEL:
			destBuffer = Malloc(size);
			buffer.address = (uintptr_t)destBuffer;
			break;
		default:
			return NULL;
	}

	buffer.type = type;
	buffer.readable = true;
	buffer.size = size;

	memset(destBuffer, 0, size);
	memcpy(destBuffer, &buffer, sizeof(buffer));

	return destBuffer;
}

uint64_t IOCtl(Buffer *buffer, BufferOperation operation, ...) {
	if (buffer == NULL) return 0;
	if (!buffer->readable) return 0;

	buffer->readable = false;

	uint64_t result;

	va_list ap;
	va_start(ap, operation);

	switch(buffer->type) {
		case COMMUNICATION_INTERKERNEL:
			result = InterkernelIOCtl(buffer, operation, ap);
			break;
		default:
			result = 0;
			break;
	}

	buffer->readable = true;

	va_end(ap);

	return result;
}

uint64_t Delete(Buffer *buffer) {
}

}
}
