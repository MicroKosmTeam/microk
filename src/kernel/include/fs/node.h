#pragma once
#include <stdint.h>

/* FSNode
 *  This permits an implementation-independent communcation between
 *  all the components of the VFS
 */
struct FSNode {
	char name[128];		// The name of the current object
	uint64_t mask;		// Permission mask
	uint64_t uid;		// User ID
	uint64_t gid;		// Group ID
	uint64_t flags;		// Node's flags (es: file, directory, symlink...)
	uint64_t inode;		// Implementation-driven inode for filesystem drivers
	uint64_t size;		// The node's size
	uint64_t impl;		// Free parameter for filesystem drivers
};

