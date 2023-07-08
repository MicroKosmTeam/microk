#pragma once
#include <stdint.h>

#include "fs.h"
#include "fops.h"

struct VNode {
	char Name[MAX_NAME_SIZE];

	filesystem_t FSDescriptor;
	inode_t Inode;
};
