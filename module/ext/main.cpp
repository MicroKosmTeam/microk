#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <cdefs.h>
#include <mkmi.h>
#include <mkmi_log.h>
#include <mkmi_memory.h>
#include <mkmi_syscall.h>

extern "C" uint32_t VendorID = 0xCAFEBABE;
extern "C" uint32_t ProductID = 0xE372C0DE;

extern "C" size_t OnMessage() {
	MKMI_Printf("Message!\r\n");
	return 0;
}

extern "C" size_t OnSignal() {
	MKMI_Printf("Signal!\r\n");
	return 0;
}

extern "C" size_t OnInit() {

	return 0;
}

extern "C" size_t OnExit() {

	return 0;
}
