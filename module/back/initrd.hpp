#pragma once
#include <cdefs.h>
#include <stdint.h>
#include <stddef.h>
#include <mkmi.h>

#include "safe_ptr.hpp"

enum InitrdFormat_t {
	INITRD_TAR_UNCOMPRESSED,
	INITRD_TAR_GZ,
	INITRD_TAR_XZ,
	INITRD_FORMAT_COUNT,
};

class InitrdInstance {
public:
	struct InitrdFile {
		safe_ptr<InitrdFile> next, prev;

		const char *name;
		usize size;
		u8 *addr;
	};

	InitrdInstance(uint8_t *address, size_t size, InitrdFormat_t format);
	InitrdInstance(const InitrdInstance& obj) : address(obj.address), size(obj.size), format(obj.format), rootFile(obj.rootFile) {}
	InitrdInstance& operator=(const InitrdInstance & dyingObj){ (void)dyingObj; return *this; }
	


	safe_ptr<InitrdFile> SearchForPath(const char *path);
private:


	int IndexInitrd();
	int UnpackInitrd();

	uint8_t *address;
	size_t size;
	InitrdFormat_t format;

	safe_ptr<InitrdFile> rootFile;
};
