#pragma once
#include <stdint.h>
#include <stddef.h>

#define MAX_NAME_SIZE            0x0100

#define NODES_IN_VNODE_TABLE     0x0040

#define NODE_CREATE              0x0001
#define NODE_DELETE              0x0002
#define NODE_GETBYNODE           0x0003
#define NODE_GETBYNAME           0x0004
#define NODE_GETBYINDEX          0x0005
#define NODE_GETROOT             0x0006
#define NODE_OPEN 		 0x0007
#define NODE_CLOSE  	         0x0008
#define NODE_READ 		 0x0009
#define NODE_WRITE 		 0x000A
#define NODE_MOUNT               0x000B
#define NODE_UNMOUNT             0x000C

#define NODE_PROPERTY_FILE       0x0001
#define NODE_PROPERTY_DIRECTORY  0x0002
#define NODE_PROPERTY_CHARDEV    0x0004
#define NODE_PROPERTY_BLOCKDEV   0x0008
#define NODE_PROPERTY_PIPE       0x0010
#define NODE_PROPERTY_SYMLINK    0x0020
#define NODE_PROPERTY_MOUNTPOINT 0x0040

#define FILE_REQUEST_MAGIC_NUMBER  0x4690738
#define FILE_RESPONSE_MAGIC_NUMBER 0x7502513

typedef uintmax_t filesystem_t;
typedef uintmax_t inode_t;
typedef uint16_t property_t;
