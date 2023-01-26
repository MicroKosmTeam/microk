#include <module/module.h>
#include <module/symbol.h>
#include <fs/vfs.h>
#include <stdio.h>
#include <mm/heap.h>
#include <mm/memory.h>
#include <elf.h>
#include <elf64.h>

#define PREFIX "[KMOD] "
ModuleManager *GlobalModuleManager;

ModuleManager::ModuleManager() {
        KRNSYM::Setup();
}

ModuleManager::~ModuleManager() {

}

bool ModuleManager::FindModule(char *moduleName) {
        //Unimplemented
        return false;
}

bool ModuleManager::LoadModule(char *moduleName) {
        //Unimplemented
        return false;
}

static bool elf_check_file(Elf64_Ehdr *hdr) {
	if(!hdr) return false;
	if(hdr->e_ident[EI_MAG0] != ELFMAG0) {
		fprintf(VFS_FILE_STDLOG, PREFIX "[ERR] " "ELF Header EI_MAG0 incorrect.\n");
		return false;
	}
	if(hdr->e_ident[EI_MAG1] != ELFMAG1) {
		fprintf(VFS_FILE_STDLOG, PREFIX "[ERR] " "ELF Header EI_MAG1 incorrect.\n");
		return false;
	}
	if(hdr->e_ident[EI_MAG2] != ELFMAG2) {
		fprintf(VFS_FILE_STDLOG, PREFIX "[ERR] " "ELF Header EI_MAG2 incorrect.\n");
		return false;
	}
	if(hdr->e_ident[EI_MAG3] != ELFMAG3) {
		fprintf(VFS_FILE_STDLOG, PREFIX "[ERR] " "ELF Header EI_MAG3 incorrect.\n");
		return false;
	}
	return true;
}

static bool elf_check_supported(Elf64_Ehdr *hdr) {
	if(!elf_check_file(hdr)) {
		fprintf(VFS_FILE_STDLOG, PREFIX "[ERR] " "Invalid ELF File.\n");
		return false;
	}
	if(hdr->e_ident[EI_CLASS] != ELFCLASS64) {
		fprintf(VFS_FILE_STDLOG, PREFIX "[ERR] " "Unsupported ELF File Class.\n");
		return false;
	}
	if(hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		fprintf(VFS_FILE_STDLOG, PREFIX "[ERR] " "Unsupported ELF File byte order.\n");
		return false;
	}
	if(hdr->e_machine != EM_AMD64) {
		fprintf(VFS_FILE_STDLOG, PREFIX "[ERR] " "Unsupported ELF File target.\n");
		return false;
	}
	if(hdr->e_ident[EI_VERSION] != EV_CURRENT) {
		fprintf(VFS_FILE_STDLOG, PREFIX "[ERR] " "Unsupported ELF File version.\n");
		return false;
	}
	if(hdr->e_type != ET_REL && hdr->e_type != ET_EXEC) {
		fprintf(VFS_FILE_STDLOG, PREFIX "[ERR] " "Unsupported ELF File type.\n");
		return false;
	}
	return true;
}

void ModuleManager::LoadELF(uint8_t *data) {
        int i = 0;

        Elf64_Ehdr *header = (Elf64_Ehdr*)malloc(sizeof(Elf64_Ehdr));
        memcpy(header, data, sizeof(Elf64_Ehdr));

        if(elf_check_file(header) && elf_check_supported(header)) {
                fprintf(VFS_FILE_STDLOG, PREFIX "ELF file is valid!\n");

        	uint64_t size = header->e_phnum * header->e_phentsize;
                fprintf(VFS_FILE_STDLOG, PREFIX "File size: %d\n", size);

        	Elf64_Phdr* phdrs = (Elf64_Phdr*)malloc(size);
                memcpy(phdrs, data + sizeof(Elf64_Ehdr), size);

        }
        
        free(header);
}
