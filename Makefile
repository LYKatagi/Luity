# ==========================================
# Luity Kernel Makefile
# ==========================================

TARGET := x86_64-elf

CC := $(TARGET)-gcc
AS := $(TARGET)-as
LD := $(TARGET)-ld
NASM := nasm

QEMU := qemu-system-x86_64
XORRISO := xorriso
LIMINE := limine

BUILD := build
ISO := iso

KERNEL_ELF := $(ISO)/boot/kernel.elf
KERNEL_ISO := $(BUILD)/kernel.iso

CFLAGS := \
	-ffreestanding \
	-Wall \
	-Wextra \
	-O2 \
	-mcmodel=kernel \
	-mno-red-zone \
	-mgeneral-regs-only \
	-Iinclude

ASFLAGS :=

LDFLAGS := \
	-T linker.ld

C_SOURCES := $(shell find kernel -type f -name '*.c')
ASM_SOURCES := $(wildcard boot/*.asm)
HEADERS := $(shell find include -type f -name '*.h')

C_OBJECTS := $(patsubst kernel/%.c,$(BUILD)/%.o,$(C_SOURCES))
ASM_OBJECTS := $(patsubst boot/%.asm,$(BUILD)/%.o,$(ASM_SOURCES))

OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

# ==========================================
# Default target
# ==========================================

.PHONY: all
all: iso

# ==========================================
# Directories
# ==========================================

$(BUILD):
	mkdir -p $(BUILD)

$(ISO)/boot:
	mkdir -p $(ISO)/boot

# ==========================================
# C → Assembly
# ==========================================

$(BUILD)/%.s: kernel/%.c $(HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -S $< -o $@
# ==========================================
# Assembly → Object
# ==========================================

$(BUILD)/%.o: $(BUILD)/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# ==========================================
# NASM → Object
# ==========================================

$(BUILD)/%.o: boot/%.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf64 $< -o $@

# ==========================================
# Kernel ELF
# ==========================================

$(KERNEL_ELF): $(OBJECTS) linker.ld | $(ISO)/boot
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

# ==========================================
# ISO
# ==========================================

.PHONY: iso
iso: $(KERNEL_ISO)

$(KERNEL_ISO): $(KERNEL_ELF) $(ISO)/boot/limine.conf \
	$(ISO)/limine-bios-cd.bin $(ISO)/limine-uefi-cd.bin \
	$(ISO)/limine-bios.sys $(ISO)/BOOTX64.EFI $(ISO)/BOOTIA32.EFI
	$(XORRISO) -as mkisofs \
    -b limine-bios-cd.bin \
    -no-emul-boot \
    -boot-load-size 4 \
    -boot-info-table \
    -eltorito-alt-boot \
    -e limine-uefi-cd.bin \
    -no-emul-boot \
    -isohybrid-gpt-basdat \
    -o $@ \
    iso
	$(LIMINE) bios-install $@

# ==========================================
# Run
# ==========================================

.PHONY: run
run: $(KERNEL_ISO)
	$(QEMU) \
		-m 128M \
		-cdrom $(KERNEL_ISO)

# ==========================================
# Clean
# ==========================================

.PHONY: clean
clean:
	rm -rf $(BUILD)
	rm -f $(ISO)/boot/kernel.elf
