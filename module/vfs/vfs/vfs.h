#pragma once

struct Inode {
	char name[128];
	uint64_t mask;
	uint64_t uid;
	uint64_t gid;
	uint64_t flags;	
	uint64_t inode;	
	uint64_t size;	
	uint64_t impl;	
};
