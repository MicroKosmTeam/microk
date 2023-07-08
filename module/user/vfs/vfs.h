#pragma once
#include <stdint.h>
#include <stddef.h>

#include "vnode.h"
#include "fs.h"
#include "fops.h"

class VirtualFilesystem {
public:
	VirtualFilesystem();
	~VirtualFilesystem();

	filesystem_t RegisterFilesystem(uint32_t vendorID, uint32_t productID);
	int DoFilesystemOperation(filesystem_t fs, FileOperationRequest *request);
	void UnregisterFilesystem(filesystem_t fs);
private:
};
