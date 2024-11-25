#include "elf_loader.hpp"
#include "heap.hpp"

ElfLoaderInstance::ElfLoaderInstance(u8 *address, usize size) : address(address), size(size), elfHeader((Elf64_Ehdr*)address) {
	if (elfHeader->e_ident[EI_MAG0] != ELFMAG0 ||
	    elfHeader->e_ident[EI_MAG1] != ELFMAG1 ||
	    elfHeader->e_ident[EI_MAG2] != ELFMAG2 ||
	    elfHeader->e_ident[EI_MAG3] != ELFMAG3) {
		/* Invalid ELF magic */
		return;
	}

	if (elfHeader->e_ident[EI_CLASS] != ELFCLASS64) {
		/* Invalid machine type */
		return;
	}

	if (elfHeader->e_type != ET_EXEC) {
		/* Not a classic executble file */
		return;
	}

	mkmi_log("Valid elf executable found\r\n");

	usize programHeaderSize = elfHeader->e_phentsize;
	usize programHeaderOffset = elfHeader->e_phoff;
	usize programHeaderNumber = elfHeader->e_phnum;

	Elf64_Phdr *programHeader;

	for (usize i = 0; i < programHeaderNumber; i++) {
		programHeader = (Elf64_Phdr*)((uptr)address + programHeaderOffset + programHeaderSize * i);

		if (programHeader->p_type != PT_LOAD) continue;

		uptr virtAddr = programHeader->p_vaddr;
		usize memSize = programHeader->p_memsz;

		uptr fileOffset = (uptr)address + programHeader->p_offset;
		usize fileSize = programHeader->p_filesz;
		usize flags = programHeader->p_flags;
		
		mkmi_log("#%d program header:\r\n"
			 " - VirtualAddress: [0x%x - 0x%x]\r\n"
			 " - FileOffset:     [0x%x - 0x%x]\r\n"
			 " - Flags:          0x%x\r\n",
			 i,
			 virtAddr, virtAddr + memSize,
			 fileOffset, fileOffset + fileSize,
			 flags);
	}

}


#ifdef UNDEF
namespace LOADER {
static bool VerifyELF(Elf64_Ehdr *elfHeader);
static int LoadProgramHeaders(u8 *data, usize size, Elf64_Ehdr *elfHeader, VirtualSpace space, uptr *highestAddress);
static usize LoadProcess(Elf64_Ehdr *elfHeader, VirtualSpace space, uptr highestAddress);

usize LoadELF(u8 *data, usize size) {
	KInfo *info = GetInfo();

	Elf64_Ehdr *elfHeader = (Elf64_Ehdr*)data;

	if(VerifyELF(elfHeader)) {
		PRINTK::PrintK(PRINTK_DEBUG "Valid ELF header.\r\n");

		VirtualSpace space = VMM::NewVirtualSpace((uptr)PMM::RequestPage());
		VMM::PrepareUserVirtualSpace(space);

		CAPABILITY::Originate(info->RootTCB->RootCNode,
				      ROOT_CNODE_SLOTS::VIRTUAL_SPACE_ROOT_SLOT,
				      space,
				      OBJECT_TYPE::PAGING_STRUCTURE,
				      CAPABILITY_RIGHTS::ACCESS);

		uptr highestAddress = 0;
		LoadProgramHeaders(data, size, elfHeader, space, &highestAddress);

		usize pid = LoadProcess(elfHeader, space, highestAddress);
		
		return pid;
	} else {
		OOPS("Invalid ELF file");
		return -1;
	}
}
static bool VerifyELF(Elf64_Ehdr *elfHeader) {
	if (elfHeader->e_ident[EI_MAG0] != ELFMAG0 ||
	    elfHeader->e_ident[EI_MAG1] != ELFMAG1 ||
	    elfHeader->e_ident[EI_MAG2] != ELFMAG2 ||
	    elfHeader->e_ident[EI_MAG3] != ELFMAG3) {
		/* Invalid ELF magic */
		return false;
	}

	if (elfHeader->e_ident[EI_CLASS] != ELFCLASS64) {
		/* Invalid machine type */
		return false;
	}

	if (elfHeader->e_type != ET_EXEC) {
		/* Not a classic executble file */
		return false;
	}

	return true;
}

static int LoadProgramHeaders(u8 *data, usize size, Elf64_Ehdr *elfHeader, VirtualSpace space, uptr *highestAddress) {
	(void)size;
	usize programHeaderSize = elfHeader->e_phentsize;
	usize programHeaderOffset = elfHeader->e_phoff;
	usize programHeaderNumber = elfHeader->e_phnum;

	Elf64_Phdr *programHeader;

	for (usize i = 0; i < programHeaderNumber; i++) {
		programHeader = (Elf64_Phdr*)(data + programHeaderOffset + programHeaderSize * i);

		if (programHeader->p_type != PT_LOAD) continue;

		uptr virtAddr = programHeader->p_vaddr;
		usize memSize = programHeader->p_memsz;

		if (*highestAddress < virtAddr + memSize) {
			*highestAddress = virtAddr + memSize;
			ROUND_UP_TO_PAGE(*highestAddress);
		}

		uptr fileOffset = (uptr)data + programHeader->p_offset;
		usize fileSize = programHeader->p_filesz;

		usize flags;

		switch (programHeader->p_flags) {
			case PF_R: /* ROData */
				flags = VMM_FLAGS_USER | VMM_FLAGS_READ | VMM_FLAGS_NOEXEC;
				break;
			case PF_R | PF_X: /* Text */
				flags = VMM_FLAGS_USER | VMM_FLAGS_READ;
				break;
			case PF_R | PF_W: /* Data or BSS */
				flags = VMM_FLAGS_USER | VMM_FLAGS_READ | VMM_FLAGS_WRITE | VMM_FLAGS_NOEXEC;
				break;
			default: /* Unknown section, just skip */
				continue;
		}
				
		VMM::VMCopyAlloc(space, virtAddr, memSize, flags, fileOffset, virtAddr, fileSize);
	}

	return 0;
}

static usize LoadProcess(Elf64_Ehdr *elfHeader, VirtualSpace space, uptr highestAddress) {
	KInfo *info = GetInfo();

	/* Get the TCB */
	ThreadControlBlock *tcb = info->RootTCB;
	tcb->MemorySpace = space;
	tcb->Priority = SCHEDULER_MAX_PRIORITY;

	uptr virtualRegistersFrame = (uptr)PMM::RequestPage();
	Memclr((void*)VMM::PhysicalToVirtual(virtualRegistersFrame), PAGE_SIZE);

	VMM::MMap(space, virtualRegistersFrame, highestAddress, PAGE_SIZE, VMM_FLAGS_READ | VMM_FLAGS_WRITE | VMM_FLAGS_USER | VMM_FLAGS_NOEXEC);

//	CAPABILITY::Originate(tcb->RootCNode,
//				      ROOT_CNODE_SLOTS::VIRTUAL_REGISTERS_FRAME_SLOT,
//				      virtualRegistersFrame,
//				      OBJECT_TYPE::FRAMES,
//				      CAPABILITY_RIGHTS::ACCESS |
//				      CAPABILITY_RIGHTS::READ |
//				      CAPABILITY_RIGHTS::WRITE);

	/* Save the mapping */
	tcb->VirtualRegisters = (u8*)highestAddress;

	info->RootVirtualRegistersFrame = virtualRegistersFrame;

	/*
	((uptr*)(VMM::PhysicalToVirtual(virtualRegistersFrame)))[0] = VMM::VirtualToPhysical(info->InitrdAddress);
	((uptr*)(VMM::PhysicalToVirtual(virtualRegistersFrame)))[1] = (info->InitrdSize);
*/
	/* Bump the highest address up one page */
	highestAddress += PAGE_SIZE;

	/* Find the stack base after the highestAddress,
	 * making sure to reserve enough space for eventual expansion
	 */
	highestAddress += INIT_MAXIMUM_STACK_GROWTH - INIT_INITIAL_STACK_SIZE;

	/* Create the capability for the stack frames
	 * Init can revoke its stack frames, if it wishes to.
	 */
	/*
	CAPABILITY::Originate(info->RootTCB->RootCNode,
				      ROOT_CNODE_SLOTS::STACK_FRAME_SLOT,
				      highestAddress,
				      INIT_INITIAL_STACK_SIZE,
				      OBJECT_TYPE::FRAMES,
				      CAPABILITY_RIGHTS::ACCESS |
				      CAPABILITY_RIGHTS::READ |
				      CAPABILITY_RIGHTS::WRITE |
				      CAPABILITY_RIGHTS::REVOKE);
	*/


	/* Allocate the space for the stack and map it in
	 * the virtual space for init
	 */
	VMM::VMAlloc(space, highestAddress, INIT_INITIAL_STACK_SIZE, VMM_FLAGS_READ | VMM_FLAGS_WRITE | VMM_FLAGS_NOEXEC | VMM_FLAGS_USER);


	/* Create the actual context */
	SchedulerContext *context = (SchedulerContext*)VMM::PhysicalToVirtual((uptr)PMM::RequestPage());
	TASK::InitializeContext((uptr)context, elfHeader->e_entry, highestAddress + INIT_INITIAL_STACK_SIZE, VIRTUAL_REGISTERS_SIZE, (uptr)tcb->VirtualRegisters);

	tcb->Context = context;

	/* Add the init thread to the boot domain scheduler */
	Scheduler *sched = info->BootDomain->DomainScheduler;
	SCHED::AddThread(sched, tcb);

	return 0;
}
}
#endif
