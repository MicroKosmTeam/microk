ARCH = x86_64
#ARCH = aarch64

CC = $(ARCH)-elf-gcc
#CC = $(ARCH)-none-elf-gcc
CPP = $(ARCH)-elf-g++
#CPP = $(ARCH)-none-elf-g++
LD = $(ARCH)-elf-ld
#LD = $(ARCH)-none-elf-ld
AR = $(ARCH)-elf-ar
#AR = $(ARCH)-none-elf-ar
AS = $(ARCH)-elf-as
#AS = $(ARCH)-none-elf-as

ASM = nasm
#ASM = $(AS)

KERNDIR = microk-kernel
MKMIDIR = mkmi

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))
