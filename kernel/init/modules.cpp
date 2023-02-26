#include <init/modules.hpp>
#include <limine.h>
#include <sys/panic.hpp>
#include <sys/printk.hpp>

static volatile limine_module_request moduleRequest {
	.id = LIMINE_MODULE_REQUEST,
	.revision = 0
};

namespace MODULE {
void Init(KInfo *info) {
	if (moduleRequest.response == NULL) PANIC("Module request not answered by Limine");

	info->modules = moduleRequest.response->modules;
	info->moduleCount = moduleRequest.response->module_count;

	PRINTK::PrintK("Available modules:\r\n");

	for (int i = 0; i < info->moduleCount; i++) {
		PRINTK::PrintK("[ %s %d ] %s\r\n",
				info->modules[i]->path,
				info->modules[i]->size,
				info->modules[i]->cmdline);
	}
}
}
