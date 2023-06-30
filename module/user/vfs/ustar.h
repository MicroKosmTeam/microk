#pragma once
#include <stdint.h>

struct TarHeader {
	char filename[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag[1];
	char linkedname[100];
	char ustar[6];
	char ustarver[2];
	char userowner[32];
	char usergroup[32];
	char devicemajor[8];
	char deviceminor[8];
	char filenamepefix[155];
}__attribute__((packed));

void LoadArchive(uint8_t *archive);
