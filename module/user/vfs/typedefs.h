#pragma once
#include <stdint.h>
#include <stddef.h>

#define MAX_NAME_SIZE 512

#define NODE_CREATE     0x0001
#define NODE_GET        0x0002
#define NODE_DELETE     0x0003
#define NODE_FINDINDIR  0x0005

typedef uintmax_t filesystem_t;
typedef uintmax_t inode_t;
