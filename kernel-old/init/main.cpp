#include <init/kutil.h>
#include <limine.h>

#define PREFIX "[KINIT] "

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent.

static volatile struct limine_bootloader_info_request bootloader_info_request = {
	.id = LIMINE_BOOTLOADER_INFO_REQUEST,
	.revision = 0
};

static volatile struct limine_stack_size_request stack_size_request = {
	.id = LIMINE_STACK_SIZE_REQUEST,
	.revision = 0,
	.stack_size = (1024 * 1024) // Requesting 1 MB stack
};

static volatile struct limine_memmap_request memmap_request = {
	.id = LIMINE_MEMMAP_REQUEST,
	.revision = 0
};

static volatile struct limine_framebuffer_request framebuffer_request = {
	.id = LIMINE_FRAMEBUFFER_REQUEST,
	.revision = 0
};

static volatile struct limine_rsdp_request rsdp_request = {
	.id = LIMINE_RSDP_REQUEST,
	.revision = 0
};

static volatile struct limine_hhdm_request hhdm_request = {
	.id = LIMINE_HHDM_REQUEST,
	.revision = 0
};

static volatile struct limine_module_request module_request = {
	.id = LIMINE_MODULE_REQUEST,
	.revision = 0
};

BootInfo bootInfo;
extern "C" int _start() {
	if (bootloader_info_request.response == NULL
		|| memmap_request.response == NULL
		|| framebuffer_request.response == NULL
		|| rsdp_request.response == NULL
		|| hhdm_request.response == NULL
		|| module_request.response == NULL) {
		return 69;
	}

	bootInfo.framebuffers = framebuffer_request.response->framebuffers;
	bootInfo.framebufferCount = framebuffer_request.response->framebuffer_count;
	bootInfo.mMap = memmap_request.response->entries;
	bootInfo.mMapEntries = memmap_request.response->entry_count;
	bootInfo.hhdmOffset = hhdm_request.response->offset;
	bootInfo.rsdp = rsdp_request.response->address;
	bootInfo.modules = module_request.response->modules;
	bootInfo.moduleCount = module_request.response->module_count;

        kinit(&bootInfo);
	//restInit();

}
