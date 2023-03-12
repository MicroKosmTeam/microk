/*
   File: sys/driver.cpp
*/

#include <sys/driver.hpp>

uint64_t Ioctl(Driver *driver, uint64_t request, ...) {
	va_list ap;
	va_start(ap, request);

	uint64_t returnVal = driver->Ioctl(request, ap);

	va_end(ap);

	return returnVal;
}
