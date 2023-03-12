/*
   File: include/sys/driver.hpp

   MicroK's driver manager
*/

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

struct Driver {
	char Name[128];

	uint64_t (*Ioctl)(uint64_t request, va_list ap);
};

uint64_t Ioctl(Driver *device, uint64_t request, ...);
