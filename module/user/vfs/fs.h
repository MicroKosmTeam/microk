#pragma once
#include "typedefs.h"
#include "fops.h"

struct Filesystem {
	filesystem_t FSDescriptor;

	uint32_t OwnerVendorID;
	uint32_t OwnerProductID;

	void *Instance;
	NodeOperations *Operations;
};
