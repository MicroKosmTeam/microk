#pragma once
#include <stdint.h>
#include <stddef.h>

struct TarHeader {
	char Filename[100];
	char Mode[8];
	char UID[8];
	char GID[8];
	char Size[12];
	char MTime[12];
	char Chksum[8];
	char TypeFlag[1];
	char LinkedName[100];
	char USTAR[6];
	char USTARVer[2];
	char UserOwner[32];
	char UserGroup[32];
	char DeviceMajor[8];
	char DeviceMinor[8];
	char FilenamePefix[155];
}__attribute__((packed));

void FindInArchive(uint8_t *archive, const char *name, uint8_t **file, size_t *size);
void LoadArchive(uint8_t *archive);
