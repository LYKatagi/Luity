# Luity Kernel Makefile

TARGET := x86_64-elf

CC := $(TARGET)-gcc
LD := $(TARGET)-ld
AS := nasm

QEMU := qemu-system-x86_64
XORRISO := xorriso

BUILD := build
ISO := iso

KERNEL_ELF := $(BUILD)/kernel.elf
KERNEL_ISO := $(BUILD)/kernel.iso

CFLAGS := 
-ffreestanding 
-Wall 
-Wextra 
-O2 
-mcmodel=kernel 
-mno-red-zone 
-Iinclude

LDFLAGS := 
-T linker.ld 
-nostdlib

C_SOURCES := $(wildcard kernel/*.c)
ASM_SOURCES := $(wildcard boot/*.asm)

C_OBJECTS := $(patsubst kernel/%.c,$(BUILD)/%.o,$(C_SOURCES))
ASM_OBJECTS := $(patsubst boot/%.asm,$(BUILD)/%.o,$(ASM_SOURCES))

OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

.PHONY: all clean iso run

all: $(KERNEL_ELF)

	$(BUILD):
	mkdir -p $(BUILD)

	$(BUILD)/%.o: kernel/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

	$(BUILD)/%.o: boot/%.asm | $(BUILD)
	$(AS) -f elf64 $< -o $@

	$(KERNEL_ELF): $(OBJECTS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

	iso: $(KERNEL_ELF)
	mkdir -p $(ISO)/boot
	cp $(KERNEL_ELF) $(ISO)/boot/kernel.elf

	
	$(XORRISO) \
		-as mkisofs \
		-b limine-bios.sys \
		-no-emul-boot \
		-bios-load-size 4 \
		-bios-load-segment 0x07C0 \
		-o $(KERNEL_ISO) \
		$(ISO)
	

run: iso
	$(QEMU) 
	-m 128M 
	-cdrom $(KERNEL_ISO)

clean:
	rm -rf $(BUILD)
	rm -f $(ISO)/boot/kernel.elf
