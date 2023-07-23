#include <stdint.h>

#include <lai/host.h>

#include <mkmi_memory.h>
#include <mkmi_heap.h>
#include <mkmi_log.h>
#include <mkmi_syscall.h>

extern "C" void *memcpy(void *dest, void *src, size_t n) {
	Memcpy(dest, src, n);
	return dest;
}

extern "C" void memset(void *start, uint8_t value, uint64_t num) {
	Memset(start, value, num);

	return start;
}

extern "C" int memcmp(const void* buf1, const void* buf2, size_t count) {
	return Memcmp(buf1, buf2, count);
}

extern "C" void laihost_log(int level, const char *msg) {
	MKMI_Printf("ACPI Log (%d): %s\r\n", level, msg);
}

extern "C" __attribute__((noreturn)) void laihost_panic(const char *msg) {
	MKMI_Printf("ACPI Panic: %s\r\n", msg);

	while(true);
}

extern "C" void *laihost_malloc(size_t size) {
	MKMI_Printf("Malloc called for size %d.\r\n", size);

	if (size == 0) {
		return NULL;
	}

	void *ptr = Malloc(size);

	if(ptr == NULL) MKMI_Printf("Warning: LAI called a NULL Malloc().\r\n");
		
	return ptr;
}

extern "C" void *laihost_realloc(void *ptr, size_t newSize, size_t oldSize) {
	MKMI_Printf("Realloc called for size %d from 0x%x to %d\r\n", oldSize, ptr, newSize);

	if (newSize == 0) {
		laihost_free(ptr, oldSize);
		return NULL;
	}

	if (newSize < oldSize) {
		return ptr;
	}

	void *newPtr = laihost_malloc(newSize);
	
	Memcpy(newPtr, ptr, newSize > oldSize ? oldSize : newSize);
	
	laihost_free(ptr, oldSize);

	return newPtr;
}

extern "C" void laihost_free(void *ptr, size_t size) {
	MKMI_Printf("Freeing\r\n");

	if (ptr == NULL) return;

	Free(ptr);
}

extern "C" void *laihost_map(size_t address, size_t count) {
	MKMI_Printf("Mapping memory: 0x%x by %d\r\n", address, count);

	return VMMap(address, address, count, 0);
}

extern "C" void *laihost_unamp(void *pointer, size_t count) {
	MKMI_Printf("Unmapping\r\n");
	
	VMUnmap(pointer, count);

	return NULL;
}

#include "../acpi.h"
extern "C" void *laihost_scan(const char *sig, size_t index) {
	MKMI_Printf("Loading table: %s\r\n", sig);
	char totalName[10];
	Memcpy(totalName, "ACPI:", 5);
	Memcpy(totalName + 5, sig, 4);
	totalName[9] = '\0';

	void *table;
	size_t size;
	Syscall(SYSCALL_FILE_OPEN, totalName, &table, &size, 0, 0, 0);

	if (table != 0 && size != 0) {
		VMMap(table, table, size, 0);
		return table;
	} else {
		return NULL;
	}
}

extern "C" void laihost_outb(uint16_t port, uint8_t val) {
	MKMI_Printf("OK\r\n");
}

extern "C" void laihost_outw(uint16_t port, uint16_t val) {
	MKMI_Printf("OK\r\n");
}

extern "C" void laihost_outd(uint16_t port, uint32_t val) {
	MKMI_Printf("OK\r\n");
}

extern "C" uint8_t laihost_inb(uint16_t port) {
	MKMI_Printf("OK\r\n");

	return 0;
}

extern "C" uint16_t laihost_inw(uint16_t port) {
	MKMI_Printf("OK\r\n");

	return 0;
}

extern "C" uint32_t laihost_ind(uint16_t port) {
	MKMI_Printf("OK\r\n");

	return 0;
}

extern "C" void laihost_pci_writeb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint8_t val) {
	MKMI_Printf("OK\r\n");
}

extern "C" void laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint16_t val) {
	MKMI_Printf("OK\r\n");
}

extern "C" void laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint32_t val) {
	MKMI_Printf("OK\r\n");
}

extern "C" uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset) {
	MKMI_Printf("OK\r\n");

	return 0;
}

extern "C" uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset) {
	MKMI_Printf("OK\r\n");
	
	return 0;
}

extern "C" uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset) {
	MKMI_Printf("OK\r\n");

	return 0;
}

extern "C" int laihost_sync_wait(struct lai_sync_state *sync, unsigned int val, int64_t timeout) {
	MKMI_Printf("OK\r\n");

	return 0;
}

extern "C" void laihost_sync_wake(struct lai_sync_state *sync) {
	MKMI_Printf("OK\r\n");
}
