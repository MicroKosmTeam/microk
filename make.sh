#!/bin/sh

make -C mkmi static && \
make -C module/microk-user-module module && \
make -C module/microk-acpi-module module && \
make -C module/microk-pci-module module && \
make -C module/microk-ahci-module module && \
make -C module/microk-usb-module module && \
make -C module/microk-doom-demo-module module && \
make -C microk-kernel kernel && \
make buildimg && \
make run-x64-efi

