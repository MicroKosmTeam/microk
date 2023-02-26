#include <arch/x64/mm/vmm.hpp>
#include <limine.h>
#include <sys/panic.hpp>
#include <mm/pmm.hpp>
#include <sys/printk.hpp>
#include <mm/memory.hpp>

#define PAGE_SIZE 0x1000

bool initialized = false;

static volatile struct limine_hhdm_request hhdmRequest {
	.id = LIMINE_HHDM_REQUEST,
	.revision = 0
};

static volatile struct limine_kernel_address_request kAddrRequest {
	.id = LIMINE_KERNEL_ADDRESS_REQUEST,
	.revision = 0
};


namespace x86_64 {
void InitVMM(KInfo *info) {
	if(initialized) return;

	initialized = true;

	if(kAddrRequest.response == NULL || hhdmRequest.response == NULL) PANIC("Virtual memory request not answered by Limine");

	info->higherHalf = hhdmRequest.response->offset;
	info->kernelPhysicalBase = kAddrRequest.response->physical_base;
	info->kernelVirtualBase = kAddrRequest.response->virtual_base;
}
}
