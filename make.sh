#!/bin/sh

gcc test.c \
	-c \
	-fPIC\
	-ffreestanding       \
	-fno-stack-protector \
	-fno-omit-frame-pointer \
	-fno-builtin-g       \
	-fno-stack-check     \
	-fno-lto             \
	-fno-pie             \
	-m64                 \
	-march=x86-64        \
	-mabi=sysv           \
	-mno-80387           \
	-mno-mmx             \
	-mno-sse             \
	-mno-sse2            \
	-mno-red-zone        \
	-mcmodel=large       \
	-fpermissive         \
	-Wno-write-strings   \
	-O0                  \
	-fno-rtti            \
	-fno-exceptions      \
	-o test.o
ld test.o \
	-nostdlib               \
	  -static                 \
	  -m elf_x86_64          \
	  -z max-page-size=0x1000 \
	  -T test.ld -o test.elf
