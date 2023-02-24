#pragma once
#include <cdefs.h>
#include <stddef.h>
#include <stdint.h>

#define VFS_NODE_FILE		0x0001
#define VFS_NODE_DIRECTORY	0x0002
#define VFS_NODE_CHARDEVICE	0x0004
#define VFS_NODE_BLOCKDEVICE	0x0006
#define VFS_NODE_FIFO           0x0008
#define VFS_NODE_PIPE		0x000A
#define VFS_NODE_SYMLINK	0x000C
#define VFS_NODE_MOUNTPOINT	0x000E

