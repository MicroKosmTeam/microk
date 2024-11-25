#pragma once
#include <cdefs.h>
#include <stdint.h>
#include <stddef.h>
#include <mkmi.h>

#include "elf.h"
#include "elf64.h"

class ElfLoaderInstance {
public:
	ElfLoaderInstance(u8 *address, usize size);
private:
	u8 *address;
	usize size;
	
	Elf64_Ehdr *elfHeader;
};
