ARCH = x86_64

KERNDIR = kernel

LDS64 = kernel64.ld
CC = $(ARCH)-elf-gcc
CPP = $(ARCH)-elf-g++
ASM = nasm
LD = $(ARCH)-elf-ld

CFLAGS = -ffreestanding       \
	 -fno-stack-protector \
	 -fno-omit-frame-pointer \
	 -fno-builtin-g       \
	 -fno-stack-check     \
	 -fno-lto             \
	 -fno-pie             \
	 -fno-pic             \
	 -m64                 \
	 -march=x86-64        \
	 -mabi=sysv           \
	 -mno-80387           \
	 -mno-mmx             \
	 -mno-sse             \
	 -mno-sse2            \
	 -mno-red-zone        \
	 -mcmodel=kernel      \
	 -I kernel/include    \
	 -fpermissive         \
	 -Wno-write-strings   \
	 -O4                  \
	 -fno-rtti            \
	 -fno-exceptions
	 #-ggdb

ASMFLAGS = -f elf64

LDFLAGS = -nostdlib               \
	  -static                 \
	  -m elf_$(ARCH)          \
	  -z max-page-size=0x1000 \
	  -T kernel/kernel.ld

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

CPPSRC = $(call rwildcard,$(KERNDIR),*.cpp)
ASMSRC = $(call rwildcard,$(KERNDIR),*.asm)
KOBJS = $(patsubst $(KERNDIR)/%.cpp, $(KERNDIR)/%.o, $(CPPSRC))
KOBJS += $(patsubst $(KERNDIR)/%.asm, $(KERNDIR)/%.o, $(ASMSRC))

kernel: $(KOBJS) link

$(KERNDIR)/%.o: $(KERNDIR)/%.cpp
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(CPP) $(CFLAGS) -c $^ -o $@


$(KERNDIR)/%.o: $(KERNDIR)/%.asm
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(ASM) $(ASMFLAGS) $^ -o $@

link: $(KOBJS)
	@ echo !==== LINKING
	$(LD) $(LDFLAGS) -o microk.elf $(KOBJS)

clean:
	@rm $(KOBJS)

buildimg: kernel
	parted -s microk.img mklabel gpt
	parted -s microk.img mkpart ESP fat32 2048s 100%
	parted -s microk.img set 1 esp on
	./limine/limine-deploy microk.img
	sudo losetup -Pf --show microk.img
	sudo mkfs.fat -F 32 /dev/loop0p1
	mkdir -p img_mount
	sudo mount /dev/loop0p1 img_mount
	sudo mkdir -p img_mount/EFI/BOOT
	sudo cp -v microk.elf \
		   limine.cfg \
		   initrd.tar \
		   limine/limine.sys \
		   img_mount/
	sudo cp -v limine/BOOTX64.EFI img_mount/EFI/BOOT/
	sync
	sudo umount img_mount
	sudo losetup -d /dev/loop0
	rm -rf img_mount

run:
	qemu-system-x86_64 -bios /usr/share/OVMF/x64/OVMF.fd \
		-m 8G \
		-chardev stdio,id=char0,logfile=serial.log,signal=off \
		-serial chardev:char0 \
		-smp sockets=1,cores=4,threads=1 -drive file="microk.img" -s -S
