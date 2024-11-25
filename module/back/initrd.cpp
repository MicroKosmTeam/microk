#include "initrd.hpp"
#include "heap.hpp"

unsigned int getsize(const char *in)
{

    unsigned int size = 0;
    unsigned int j;
    unsigned int count = 1;

    for (j = 11; j > 0; j--, count *= 8)
        size += ((in[j - 1] - '0') * count);

    return size;

}

InitrdInstance::InitrdInstance(uint8_t *address, size_t size, InitrdFormat_t format)  : address(address), size(size), format(format), rootFile() {
	if (format >= INITRD_FORMAT_COUNT) {
		format = INITRD_TAR_UNCOMPRESSED;
	}

	rootFile = secure_malloc<InitrdFile>();
	rootFile->name = "";
	rootFile->size = 0;
	rootFile->addr = NULL;
	rootFile->prev = NULL;

	if (format == INITRD_TAR_UNCOMPRESSED) {
		// Can use it as is
		IndexInitrd();
	} else {
		// We must unpack it
	}
}

struct tar_header
{
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag[1];
};

int InitrdInstance::IndexInitrd() {
    unsigned char *ptr = address;

    if(memcmp(ptr + 257, "ustar", 5)) {
	    return -1;
    }

    safe_ptr<InitrdFile> file = rootFile;
    // TODO: Without that 4096 we crash...to be continued
    while((uptr)ptr - (uptr)address < size - 4096 && !memcmp(ptr + 257, "ustar", 5)) {
	tar_header *tarFile = (tar_header*)ptr;
        int filesize = getsize(tarFile->size);

	file->next = secure_malloc<InitrdFile>();
	file->next->prev = file;
	file = file->next;
	file->next = NULL;

	file->name = tarFile->filename;
	file->size = filesize;
	file->addr = (u8*)((uptr)ptr + 512);

	mkmi_log("File: %s of size %d bytes\r\n", file->name, filesize);

	/*
        if (!memcmp(ptr, filename, strlen(filename) + 1)) {
            *out = ptr + 512;
            return filesize;
        }*/

        ptr += (((filesize + 511) / 512) + 1) * 512;
    }
	
    mkmi_log("OK\r\n");

    return 0;
}
	
safe_ptr<InitrdInstance::InitrdFile> InitrdInstance::SearchForPath(const char *path) {
    safe_ptr<InitrdFile> file = rootFile;

    while(file.get()) {
        if (strcmp(file->name, path) == 0) {
		return file;
	}
	
	file = file->next;
    }

    return NULL;
}

int InitrdInstance::UnpackInitrd() {
	return -1;
}

