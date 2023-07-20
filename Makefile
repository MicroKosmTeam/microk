include Makefile.inc

.PHONY: nconfig menuconfig initrd buildimg run-arm run-x64-efi run-x64-bios

compiler:
	@ cd ./compiler/
	@ ./compile.sh x86_64-elf
	@ cd ..

nconfig:
	@ ./config/Configure ./config/config.in
	@ cp config/autoconf.h $(KERNDIR)/src/include/autoconf.h

menuconfig:
	@ ./config/Menuconfig ./config/config.in
	@ cp config/autoconf.h $(KERNDIR)/src/include/autoconf.h

initrd:
	@ mkdir -p tmp_initrd/init
	@ cp module/*.elf tmp_initrd/init
	@ rm -f tmp_initrd/init/user.elf
	@ cd tmp_initrd && tar -cvf ../initrd.tar * && cd ..
	@ rm -rf tmp_initrd

buildimg: initrd
	parted -s microk.img mklabel gpt
	parted -s microk.img mkpart ESP fat32 2048s 100%
	parted -s microk.img set 1 esp on
	sudo losetup -Pf --show microk.img
	sudo mkfs.fat -F 32 /dev/loop0p1
	mkdir -p img_mount
	sudo mount /dev/loop0p1 img_mount
	sudo mkdir -p img_mount/EFI/BOOT
	sudo cp -v $(KERNDIR)/microk.elf \
		   module/*.elf \
		   limine.cfg \
		   initrd.tar \
		   img_mount/
	sudo cp -v limine/*.EFI img_mount/EFI/BOOT/
	sync
	sudo umount img_mount
	sudo losetup -d /dev/loop0
	rm -rf img_mount

run-arm:
	qemu-system-aarch64 \
		-machine virt \
		-bios /usr/share/OVMF/aarch64/QEMU_CODE.fd  \
		-m 1G \
		-cpu cortex-a57 \
		-chardev stdio,id=char0,logfile=serial.log,signal=off \
		-serial chardev:char0 \
		-smp sockets=1,cores=4,threads=1 \
		-drive file="microk.img" \
		-device qemu-xhci \
		-s \
		-S

run-x64-bios:
	qemu-system-x86_64 \
		-M hpet=on \
		-m 1G \
		-chardev stdio,id=char0,logfile=serial.log,signal=off \
		-serial chardev:char0 \
		-smp sockets=1,cores=4,threads=1 \
		-drive file="microk.img" \
		-s \
		-machine type=q35 \
		-S \
		-device qemu-xhci

run-x64-efi:
	qemu-system-x86_64 \
		-bios /usr/share/OVMF/x64/OVMF_CODE.fd \
		-M hpet=on \
		-m 1G \
		-chardev stdio,id=char0,logfile=serial.log,signal=off \
		-serial chardev:char0 \
		-smp sockets=1,cores=4,threads=1 \
		-drive file="microk.img" \
		-machine type=q35 \
		-device qemu-xhci \
		-s \
		-S

