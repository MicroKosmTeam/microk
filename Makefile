OSNAME = microk
ARCH = x86_64

SRCDIR = src
OBJDIR = obj
BINDIR = bin

KERNDIR = $(SRCDIR)/kernel
EFIDIR = $(SRCDIR)/gnu-efi
DOOMDIR = $(SRCDIR)/microk-doom
BOOTEFI := $(EFIDIR)/x86_64/bootloader/main.efi

OVMFDIR = OVMFbin
LDS64 = kernel64.ld
CC = gcc
CPP = g++
ASMC = nasm
LD = ld

CFLAGS = -ffreestanding -fshort-wchar -fno-stack-protector -mno-red-zone -fno-exceptions -Wall -I src/kernel/include
ASMFLAGS = -f elf64
LDFLAGS = -T $(LDS64) -static -Bsymbolic -nostdlib

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

KSRC = $(call rwildcard,$(KERNDIR),*.cpp)
KASMSRC  = $(call rwildcard,$(KERNDIR),*.asm)
KOBJS = $(patsubst $(KERNDIR)/%.cpp, $(OBJDIR)/kernel/%.o, $(KSRC))
KOBJS += $(patsubst $(KERNDIR)/%.asm, $(OBJDIR)/kernel/%_asm.o, $(KASMSRC))
DIRS = $(wildcard $(SRCDIR)/*)

kernel: setup bootloader $(KOBJS) link

$(OBJDIR)/kernel/cpu/interrupts/interrupts.o: $(KERNDIR)/cpu/interrupts/interrupts.cpp
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(CPP) -mgeneral-regs-only $(CFLAGS) -c $^ -o $@

$(OBJDIR)/kernel/%.o: $(KERNDIR)/%.cpp
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(CPP) $(CFLAGS) -c $^ -o $@

$(OBJDIR)/kernel/%_asm.o: $(KERNDIR)/%.asm
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(ASMC) $(ASMFLAGS) $^ -o $@

bootloader:
	make -C $(EFIDIR)
	make -C $(EFIDIR) bootloader

doom:
	make -C $(DOOMDIR)

link:
	@ echo !==== LINKING
	$(LD) $(LDFLAGS) -o $(BINDIR)/kernel.elf $(KOBJS)

setup:
	@mkdir -p $(BINDIR)
	@mkdir -p $(SRCDIR)
	@mkdir -p $(OBJDIR)

clean:
	@rm -rf $(OBJDIR)

buildimg: kernel
	dd if=/dev/zero of=$(BINDIR)/$(OSNAME).img bs=512 count=93750
	mformat -i $(BINDIR)/$(OSNAME).img ::
	mmd -i $(BINDIR)/$(OSNAME).img ::/EFI
	mmd -i $(BINDIR)/$(OSNAME).img ::/EFI/BOOT
	mcopy -i $(BINDIR)/$(OSNAME).img $(BOOTEFI) ::/EFI/BOOT
	mcopy -i $(BINDIR)/$(OSNAME).img startup.nsh ::
	mcopy -i $(BINDIR)/$(OSNAME).img $(BINDIR)/kernel.elf ::
	mcopy -i $(BINDIR)/$(OSNAME).img $(BINDIR)/zap-light16.psf ::

run: buildimg
	qemu-system-x86_64 -drive file=$(BINDIR)/$(OSNAME).img -machine q35 -m 6G -cpu qemu64 -drive if=pflash,format=raw,unit=0,file="$(OVMFDIR)/OVMF_CODE-pure-efi.fd",readonly=on -drive if=pflash,format=raw,unit=1,file="$(OVMFDIR)/OVMF_VARS-pure-efi.fd" -net none
