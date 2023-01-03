OSNAME = microk
ARCH = x86_64

SRCDIR = src
OBJDIR = obj
BINDIR = bin

EFIDIR = $(SRCDIR)/gnu-efi
BOOTEFI := $(EFIDIR)/x86_64/bootloader/main.efi

OVMFDIR = OVMFbin
LDS64 = kernel64.ld
CC = gcc
CPP = g++
LD = ld

CFLAGS = -ffreestanding -fshort-wchar -fno-stack-protector -Wall -I src/kernel/include
LDFLAGS = -T $(LDS64) -static -Bsymbolic -nostdlib

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRC = $(call rwildcard,$(SRCDIR),*.cpp)          
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC))
DIRS = $(wildcard $(SRCDIR)/*)

kernel: setup bootloader $(OBJS) link

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(CPP) $(CFLAGS) -c $^ -o $@

bootloader:
	make -C $(EFIDIR)
	make -C $(EFIDIR) bootloader

link:
	@ echo !==== LINKING
	$(LD) $(LDFLAGS) -o $(BINDIR)/kernel.elf $(OBJS)

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
	qemu-system-x86_64 -drive file=$(BINDIR)/$(OSNAME).img -m 6G -cpu qemu64 -drive if=pflash,format=raw,unit=0,file="$(OVMFDIR)/OVMF_CODE-pure-efi.fd",readonly=on -drive if=pflash,format=raw,unit=1,file="$(OVMFDIR)/OVMF_VARS-pure-efi.fd" -net none
