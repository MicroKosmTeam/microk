include ../../Makefile.inc

MODDIR = .
MODNAME = acpi

CFLAGS = -ffreestanding -fno-stack-protector -fno-omit-frame-pointer -fno-builtin-g -fno-stack-check -I../../mkmi/src/include -Iacpi/lai/include -I../../microk-kernel/src/include -m64 -mabi=sysv -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone -Wall -Wno-write-strings -Og -fno-exceptions -fno-lto -fno-pie -fno-pic -march=x86-64 -ggdb

CXXFLAGS := $(CFLAGS)
CXXFLAGS += -fpermissive -fno-rtti


ASMFLAGS = -f elf64

LDFLAGS = -static \
	  -Ttext 0x1000000 \
	  -nostdlib               \
	  -m elf_$(ARCH)          \
	  -z max-page-size=0x1000

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

CSRC = $(call rwildcard,$(MODDIR),*.c)
CPPSRC = $(call rwildcard,$(MODDIR),*.cpp)
ASMSRC = $(call rwildcard,$(MODDIR),*.asm)
OBJS = $(patsubst $(MODDIR)/%.c, $(MODDIR)/%.o, $(CSRC))
OBJS += $(patsubst $(MODDIR)/%.cpp, $(MODDIR)/%.o, $(CPPSRC))
OBJS += $(patsubst $(MODDIR)/%.asm, $(MODDIR)/%.o, $(ASMSRC))

$(MODDIR)/%.o: $(MODDIR)/%.c
	@ mkdir -p $(@D)
	@ echo !==== COMPILING MODULE $^ && \
	$(CC) $(CFLAGS) -c $^ -o $@

$(MODDIR)/%.o: $(MODDIR)/%.cpp
	@ mkdir -p $(@D)
	@ echo !==== COMPILING MODULE $^ && \
	$(CPP) $(CXXFLAGS) -c $^ -o $@


$(MODDIR)/%.o: $(MODDIR)/%.asm
	@ mkdir -p $(@D)
	@ echo !==== COMPILING MODULE $^  && \
	$(ASM) $(ASMFLAGS) $^ -o $@

static: $(OBJS)
	@ echo !==== LINKING
	$(LD) $(LDFLAGS) -o ../$(MODNAME).elf $(OBJS) -L. -lmkmi

dynamic: $(OBJS)
	@ echo !==== LINKING
	$(LD) $(LDFLAGS) -o ../$(MODNAME).elf $(OBJS)

clean:
	@rm $(OBJS)
