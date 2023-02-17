OSNAME = microk
ARCH = x86_64

SRCDIR = src
OBJDIR = obj
BINDIR = bin

KERNDIR = $(SRCDIR)/kernel
EFIDIR = $(SRCDIR)/gnu-efi
MODDIR = $(SRCDIR)/module
DOOMDIR = $(SRCDIR)/microk-kdoom/doomgeneric
BOOTEFI := $(EFIDIR)/x86_64/bootloader/main.efi

OVMFDIR = OVMFbin
LDS64 = kernel64.ld
MODLDS64 = module64.ld
CC = x86_64-elf-gcc
CPP = x86_64-elf-g++
ASMC = nasm
LD = x86_64-elf-gcc

CFLAGS = -mcmodel=large -fno-builtin-g -ffreestanding -fshort-wchar -fstack-protector-all -mno-red-zone -fno-omit-frame-pointer -Wall -I src/kernel/include -fsanitize=undefined -fno-exceptions -fpermissive -fno-rtti -O3 -std=c++17 -D$(ARCH)
ASMFLAGS = -f elf64
LDFLAGS = -T $(LDS64) -static -Bsymbolic -nostdlib
MODLDFLAGS = -T $(MODLDS64) -static -Bsymbolic -nostdlib

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

KSRC = $(call rwildcard,$(KERNDIR),*.cpp)
KASMSRC = $(call rwildcard,$(KERNDIR),*.asm)
KCSRC = $(call rwildcard,$(KERNDIR),*.c)
MSRC =  $(call rwildcard,$(MODDIR),*.cpp)
KOBJS = $(patsubst $(KERNDIR)/%.cpp, $(OBJDIR)/kernel/%.o, $(KSRC))
KOBJS += $(patsubst $(KERNDIR)/%.c, $(OBJDIR)/kernel/%_c.o, $(KCSRC))
KOBJS += $(patsubst $(KERNDIR)/%.asm, $(OBJDIR)/kernel/%_asm.o, $(KASMSRC))
MOBJS = $(patsubst $(MODDIR)/%.cpp, $(OBJDIR)/module/%.o, $(MSRC))
DIRS = $(wildcard $(SRCDIR)/*)

kernel: setup bootloader $(KOBJS) link symbols link-again $(MOBJS) #link-module

$(OBJDIR)/kernel/cpu/interrupts/interrupts.o: $(KERNDIR)/cpu/interrupts/interrupts.cpp
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(CPP) -mgeneral-regs-only $(CFLAGS) -c $^ -o $@

$(OBJDIR)/kernel/%.o: $(KERNDIR)/%.cpp
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(CPP) $(CFLAGS) -c $^ -o $@

$(OBJDIR)/kernel/%_c.o: $(KERNDIR)/%.c
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $^ -o $@

$(OBJDIR)/kernel/%_asm.o: $(KERNDIR)/%.asm
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(ASMC) $(ASMFLAGS) $^ -o $@

$(OBJDIR)/module/%.o: $(MODDIR)/%.cpp
	@ echo !==== COMPILING MODULE $^
	@ mkdir -p $(@D)
	$(CPP) $(CFLAGS) -c $^ -o $@

bootloader:
	make -C $(EFIDIR)
	make -C $(EFIDIR) bootloader

doom-clean:
	make -C $(DOOMDIR) clean

doom:
	make -C $(DOOMDIR)

link-module: $(MOBJS)
	@ echo !==== MODULE
	$(LD) $(MODLDFLAGS) -o $(BINDIR)/module.elf $(MOBJS)

link: $(KOBJS)
	@ echo !==== LINKING
	$(LD) $(LDFLAGS) -o $(BINDIR)/kernel-tmp.elf $(KOBJS)

symbols: link
	bash ./symbolstocpp.sh

link-again: $(KOBJS)
	@ echo !==== LINKING
	 $(LD) $(LDFLAGS) -o $(BINDIR)/kernel.elf $(KOBJS)
	 strip -s $(BINDIR)/kernel.elf

setup:
	@mkdir -p $(BINDIR)
	@mkdir -p $(SRCDIR)
	@mkdir -p $(OBJDIR)

clean:
	@rm -rf $(OBJDIR)

buildimg: kernel
	dd if=/dev/zero of=$(BINDIR)/$(OSNAME).img bs=512 count=250000
	mformat -F -v "Microk" -i $(BINDIR)/$(OSNAME).img ::
	mmd -i $(BINDIR)/$(OSNAME).img ::/EFI
	mmd -i $(BINDIR)/$(OSNAME).img ::/EFI/BOOT
	mcopy -i $(BINDIR)/$(OSNAME).img $(BINDIR)/initrd.tar ::
	mcopy -i $(BINDIR)/$(OSNAME).img startup.nsh ::
	mcopy -i $(BINDIR)/$(OSNAME).img $(BOOTEFI) ::/EFI/BOOT
	mcopy -i $(BINDIR)/$(OSNAME).img $(BINDIR)/kernel.elf ::
	mcopy -i $(BINDIR)/$(OSNAME).img $(BINDIR)/zap-light16.psf ::

run: buildimg
	qemu-system-x86_64 -serial file:serial.log -drive file=$(BINDIR)/$(OSNAME).img -machine q35 -m 6G -cpu qemu64 -smp sockets=1,cores=4,threads=1 -drive if=pflash,format=raw,unit=0,file="$(OVMFDIR)/OVMF_CODE-pure-efi.fd",readonly=on -drive if=pflash,format=raw,unit=1,file="$(OVMFDIR)/OVMF_VARS-pure-efi.fd"
run-tty: buildimg
	qemu-system-x86_64 -drive file=$(BINDIR)/$(OSNAME).img -machine q35 -m 6G -cpu qemu64 -smp sockets=1,cores=4,threads=1 -drive if=pflash,format=raw,unit=0,file="$(OVMFDIR)/OVMF_CODE-pure-efi.fd",readonly=on -drive if=pflash,format=raw,unit=1,file="$(OVMFDIR)/OVMF_VARS-pure-efi.fd" -nographic
