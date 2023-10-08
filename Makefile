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
	@ cp module/*.elf base/modules
	@ cd base && tar -cvf ../initrd.tar * && cd ..

LOOPDEV=$(shell losetup -f)

buildimg: initrd
	parted -s microk.img mklabel gpt
	parted -s microk.img mkpart ESP fat32 2048s 100%
	parted -s microk.img set 1 esp on
	sudo losetup -Pf --show microk.img
	sudo mkfs.fat -F 32 $(LOOPDEV)p1
	mkdir -p img_mount
	sudo mount $(LOOPDEV)p1 img_mount
	sudo mkdir -p img_mount/EFI/BOOT
	sudo cp -v $(KERNDIR)/microk.elf \
		   limine/limine-bios.sys \
		   module/*.elf \
		   limine.cfg \
		   initrd.tar \
		   img_mount/
	sudo cp -v limine/*.EFI img_mount/EFI/BOOT/
	sync
	sudo umount img_mount
	sudo losetup -d $(LOOPDEV)
	./limine/limine bios-install microk.img
	rm -rf img_mount

run-arm:
	qemu-system-aarch64 \
		-bios /usr/share/OVMF/aarch64/QEMU_CODE.fd  \
		-m 1G \
		-chardev stdio,id=char0,logfile=serial.log,signal=off \
		-serial chardev:char0 \
		-smp sockets=1,cores=4,threads=1 \
		-machine virt \
		-cpu cortex-a57 \
		-device qemu-xhci \
		-net nic,model=virtio \
		-device virtio-blk-pci,drive=drive0 \
		-drive id=drive0,if=none,file="microk.img" \
		-d guest_errors \
		-s \
		-S

run-x64-bios:
	qemu-system-x86_64 \
		-M hpet=on \
		-m 1G \
		-accel tcg \
		-chardev stdio,id=char0,logfile=serial.log,signal=off \
		-serial chardev:char0 \
		-smp sockets=1,cores=4,threads=1 \
		-machine type=q35 \
		-device qemu-xhci \
		-net nic,model=virtio \
		-device virtio-blk-pci,drive=drive0 \
		-drive id=drive0,if=none,file="microk.img" \
		-device ich9-intel-hda \
		-device hda-micro \
		-device sb16 \
		-device usb-mouse \
		-vga virtio \
		-d cpu_reset \
		-d guest_errors \
		-s \
		-S

run-x64-efi:
	qemu-system-x86_64 \
		-bios /usr/share/OVMF/x64/OVMF_CODE.fd \
		-M hpet=on \
		-m 1G \
		-accel tcg \
		-chardev stdio,id=char0,logfile=serial.log,signal=off \
		-serial chardev:char0 \
		-smp sockets=1,cores=4,threads=1 \
		-machine type=q35 \
		-device qemu-xhci \
		-net nic,model=virtio \
		-device virtio-blk-pci,drive=drive0 \
		-drive id=drive0,if=none,file="microk.img" \
		-device ich9-intel-hda \
		-device hda-micro \
		-device sb16 \
		-device usb-mouse \
		-vga virtio \
		-d cpu_reset \
		-d guest_errors \
		-s \
		-S
