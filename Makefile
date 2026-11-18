CC  = ~/opt/cross/bin/x86_64-elf-gcc
CXX = ~/opt/cross/bin/x86_64-elf-g++
LD  = ~/opt/cross/bin/x86_64-elf-ld
OBJ = ~/opt/cross/bin/x86_64-elf-objcopy

GCC_INCLUDE = /home/renee/opt/cross/bin/lib/gcc/x86_64-elf/15.2.0/include
GXX_INCLUDE = /home/renee/opt/cross/bin/x86_64-elf/include/c++/15.2.0

S2_CFLAGS = -ffreestanding -m16 -mgeneral-regs-only -O1 -I$(GCC_INCLUDE)

CXXFLAGS = -ffreestanding -Wall -Wextra -fno-exceptions \
		   -fno-rtti -m16 -mgeneral-regs-only \
		   -I./boot/s3/include -DBUILD_DEBUG -I$(GCC_INCLUDE) -I$(GXX_INCLUDE) \
		   -fno-omit-frame-pointer -fno-stack-protector \
		   -fno-pie -nostdlib -mno-red-zone

BUILD_BIN = build

# ---- Stage 1 ----
BOOT1_ASM = ./boot/s1.s
BOOT1_BIN = ./$(BUILD_BIN)/s1.bin

# ---- Stage 2 ----
BOOT2_ASM     = ./boot/s2/s2.s
BOOT2_C       = ./boot/s2/s2.c
BOOT2_HELPASM = ./boot/s2/s2func.s
BOOT2_LD      = ./boot/s2/s2.ld

BOOT2_OBJ  = ./$(BUILD_BIN)/s2.o
BOOT2_COBJ = ./$(BUILD_BIN)/s2.c.o
BOOT2_HLP  = ./$(BUILD_BIN)/s2func.o
BOOT2_ELF  = ./$(BUILD_BIN)/s2.elf
BOOT2_BIN  = ./$(BUILD_BIN)/s2.bin

# ---- Stage 3 ----
BOOT3_ASM    = ./boot/s3/s3.s
BOOT3_C      = ./boot/s3/s3.cpp
BOOT3_D      = $(wildcard ./boot/s3/drivers/*.cpp)
BOOT3_DRVASM = $(wildcard ./boot/s3/drivers/*.s)
BOOT3_LD     = ./boot/s3/s3.ld

BOOT3_OBJ  = ./$(BUILD_BIN)/s3.o
BOOT3_COBJ = ./$(BUILD_BIN)/s3.cpp.o
BOOT3_DRIV = $(patsubst ./boot/s3/drivers/%.cpp,./$(BUILD_BIN)/%.cpp.o,$(BOOT3_D))
BOOT3_DRVO = $(patsubst ./boot/s3/drivers/%.s,./$(BUILD_BIN)/%.o,$(BOOT3_DRVASM))
BOOT3_ELF  = ./$(BUILD_BIN)/s3.elf
BOOT3_BIN  = ./$(BUILD_BIN)/s3.bin

# ---- Outputs ----
BOOT_BIN = sbl.bin
DISK_IMG = sbl.img

# Optional: a kernel ELF to drop into /BOOT/ on the test image.
# SBL asks for the path at boot, so this is purely for convenience.
#   make KERNEL=path/to/kernel.elf
#   make KERNEL=path/to/kernel.elf KERNEL_NAME=FOO.ELF
KERNEL      ?=
KERNEL_NAME ?= $(shell basename "$(KERNEL)" | tr a-z A-Z)

ifdef S2_DEBUG
    S2_DEBUG_FLAG := -D$(S2_DEBUG)
endif

all: $(BUILD_BIN) $(DISK_IMG)

$(BUILD_BIN):
	mkdir -p $(BUILD_BIN)

# ---- Stage 1 ----
$(BOOT1_BIN): $(BOOT1_ASM) | $(BUILD_BIN)
	nasm -f bin $< -o $@

# ---- Stage 2 ----
$(BOOT2_OBJ): $(BOOT2_ASM) | $(BUILD_BIN)
	nasm -f elf32 $< -o $@

$(BOOT2_HLP): $(BOOT2_HELPASM) | $(BUILD_BIN)
	nasm -f elf32 $< -o $@

$(BOOT2_COBJ): $(BOOT2_C) | $(BUILD_BIN)
	$(CC) $(S2_CFLAGS) $(S2_DEBUG_FLAG) -c $< -o $@

$(BOOT2_ELF): $(BOOT2_OBJ) $(BOOT2_COBJ) $(BOOT2_HLP) $(BOOT2_LD)
	$(LD) -m elf_i386 -T $(BOOT2_LD) -o $@ $(BOOT2_OBJ) $(BOOT2_COBJ) $(BOOT2_HLP) -Map=$(BUILD_BIN)/s2.map

$(BOOT2_BIN): $(BOOT2_ELF)
	$(OBJ) -O binary $< $@

# ---- Stage 3 ----
$(BOOT3_OBJ): $(BOOT3_ASM) | $(BUILD_BIN)
	nasm -f elf32 $< -o $@

./$(BUILD_BIN)/%.o: ./boot/s3/drivers/%.s | $(BUILD_BIN)
	nasm -f elf32 $< -o $@

$(BOOT3_COBJ): $(BOOT3_C) | $(BUILD_BIN)
	$(CXX) $(CXXFLAGS) -c $< -o $@

./$(BUILD_BIN)/%.cpp.o: ./boot/s3/drivers/%.cpp | $(BUILD_BIN)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BOOT3_ELF): $(BOOT3_OBJ) $(BOOT3_COBJ) $(BOOT3_DRIV) $(BOOT3_DRVO) $(BOOT3_LD)
	$(LD) -m elf_i386 -T $(BOOT3_LD) -o $@ $(BOOT3_OBJ) $(BOOT3_COBJ) $(BOOT3_DRIV) $(BOOT3_DRVO) -Map=$(BUILD_BIN)/s3.map

$(BOOT3_BIN): $(BOOT3_ELF)
	$(OBJ) -O binary $< $@

# ---- Bootloader blob ----
$(BOOT_BIN): $(BOOT1_BIN) $(BOOT2_BIN) $(BOOT3_BIN)
	cat $^ > $@

# ---- Test disk image (MBR + FAT32 partition, SBL in the gap before it) ----
$(DISK_IMG): $(BOOT_BIN) $(KERNEL)
	dd if=/dev/zero of=$@ bs=1M count=512
	printf 'label: dos\nstart=2048, type=c, bootable\n' | sfdisk $@
	mkfs.fat -F 32 -n "SBL" --offset=2048 $@
	dd if=$(BOOT_BIN) of=$@ bs=446 count=1 conv=notrunc
	dd if=$(BOOT_BIN) of=$@ bs=512 skip=1 seek=1 conv=notrunc
	mmd -i $@@@1048576 ::/BOOT
ifneq ($(strip $(KERNEL)),)
	mcopy -i $@@@1048576 $(KERNEL) ::/BOOT/$(KERNEL_NAME)
endif

run: $(DISK_IMG)
	qemu-system-x86_64 -drive format=raw,file=$(DISK_IMG),if=ide -serial stdio -m 6G -d int,cpu_reset -no-reboot -D qemu.log -cpu host -accel kvm

debug: $(DISK_IMG)
	qemu-system-x86_64 -drive format=raw,file=$(DISK_IMG),if=ide -serial stdio -s -S -m 1G -accel kvm -cpu host,+rdrand

clean:
	rm -rf $(BUILD_BIN) $(BOOT_BIN) $(DISK_IMG) qemu.log

.PHONY: all run debug clean
