#!/usr/bin/env bash

# ============================================================
# Luity - Environment Diagnostic
# ============================================================

set +e

echo "============================================================"
echo "                 LUITY ENVIRONMENT DIAGNOSTIC"
echo "============================================================"
echo

# ------------------------------------------------------------
# Helpers
# ------------------------------------------------------------

section() {
    echo
    echo "------------------------------------------------------------"
    echo "$1"
    echo "------------------------------------------------------------"
}

check_command() {
    local cmd="$1"

    if command -v "$cmd" >/dev/null 2>&1; then
        echo "[OK]      $cmd -> $(command -v "$cmd")"
    else
        echo "[MISSING] $cmd"
    fi
}

version_command() {
    local name="$1"
    shift

    echo
    echo "[$name]"
    "$@" 2>&1 | head -n 5
}

# ------------------------------------------------------------
# OS / Shell
# ------------------------------------------------------------

section "OS / SHELL"

echo "uname:"
uname -a

echo
echo "SHELL=${SHELL:-<unset>}"
echo "MSYSTEM=${MSYSTEM:-<unset>}"
echo "MSYS2_PATH_TYPE=${MSYS2_PATH_TYPE:-<unset>}"

echo
echo "Current directory:"
pwd

echo
echo "Windows username:"
whoami 2>/dev/null

# ------------------------------------------------------------
# PATH
# ------------------------------------------------------------

section "PATH"

echo "$PATH" | tr ':' '\n'

# ------------------------------------------------------------
# Commands
# ------------------------------------------------------------

section "COMMAND AVAILABILITY"

COMMANDS=(
    git
    make
    gcc
    x86_64-elf-gcc
    x86_64-elf-ld
    x86_64-elf-as
    x86_64-elf-readelf
    nasm
    xorriso
    limine
    qemu-system-x86_64
    objdump
    readelf
)

for cmd in "${COMMANDS[@]}"; do
    check_command "$cmd"
done

# ------------------------------------------------------------
# Versions
# ------------------------------------------------------------

section "VERSIONS"

version_command "Git" git --version
version_command "Make" make --version
version_command "Cross GCC" x86_64-elf-gcc --version
version_command "Cross LD" x86_64-elf-ld --version
version_command "Cross AS" x86_64-elf-as --version
version_command "Cross readelf" x86_64-elf-readelf --version
version_command "NASM" nasm --version
version_command "Xorriso" xorriso --version
version_command "Limine" limine --version
version_command "QEMU" qemu-system-x86_64 --version

# ------------------------------------------------------------
# Tool locations
# ------------------------------------------------------------

section "TOOL LOCATIONS"

for cmd in \
    git \
    make \
    x86_64-elf-gcc \
    x86_64-elf-ld \
    x86_64-elf-as \
    x86_64-elf-readelf \
    nasm \
    xorriso \
    limine \
    qemu-system-x86_64
do
    echo
    echo "[$cmd]"

    if command -v "$cmd" >/dev/null 2>&1; then
        command -v "$cmd"
    else
        echo "NOT FOUND"
    fi
done

# ------------------------------------------------------------
# Git
# ------------------------------------------------------------

section "GIT"

if command -v git >/dev/null 2>&1; then

    echo "Git version:"
    git --version

    echo
    echo "Current branch:"
    git branch --show-current

    echo
    echo "Status:"
    git status --short --branch

    echo
    echo "Recent commits:"
    git log --oneline --decorate -10

    echo
    echo "Remote:"
    git remote -v

else
    echo "[WARNING] Git is not available in this shell."
fi

# ------------------------------------------------------------
# Project structure
# ------------------------------------------------------------

section "LUITY PROJECT STRUCTURE"

FILES=(
    "Makefile"
    "linker.ld"
    "README.md"
    "CHANGELOG.md"
    "kernel/main.c"
    "kernel/graphics.c"
    "kernel/font8x8.c"
    "kernel/limine_framebuffer.c"
    "include/graphics.h"
    "include/font8x8.h"
    "include/limine_framebuffer.h"
    "include/limine.h"
    "boot/boot.asm"
    "iso/boot/limine.conf"
    "iso/limine-bios-cd.bin"
    "iso/limine-uefi-cd.bin"
    "iso/limine-bios.sys"
    "iso/BOOTX64.EFI"
    "iso/BOOTIA32.EFI"
)

for file in "${FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "[OK]      $file"
    else
        echo "[MISSING] $file"
    fi
done

# ------------------------------------------------------------
# Directory structure
# ------------------------------------------------------------

section "DIRECTORIES"

for dir in \
    kernel \
    include \
    boot \
    iso \
    iso/boot \
    build
do
    if [ -d "$dir" ]; then
        echo "[OK]      $dir/"
    else
        echo "[MISSING] $dir/"
    fi
done

# ------------------------------------------------------------
# Source files
# ------------------------------------------------------------

section "SOURCE FILES"

echo "C files:"
find kernel -type f -name '*.c' 2>/dev/null

echo
echo "ASM files:"
find boot -type f \( -name '*.asm' -o -name '*.s' \) 2>/dev/null

echo
echo "Headers:"
find include -type f -name '*.h' 2>/dev/null


# ------------------------------------------------------------
# Makefile inspection
# ------------------------------------------------------------

section "MAKEFILE"

if [ -f Makefile ]; then

    echo "Compiler:"
    grep -E '^[[:space:]]*(CC|LD|AS|NASM)[[:space:]]*:=' Makefile

    echo
    echo "Flags:"
    grep -E '^[[:space:]]*(CFLAGS|ASFLAGS|LDFLAGS)[[:space:]]*:=' Makefile

    echo
    echo "Sources:"
    grep -E '^[[:space:]]*(C_SOURCES|ASM_SOURCES|OBJECTS)[[:space:]]*:=' Makefile

else
    echo "Makefile not found."
fi

# ------------------------------------------------------------
# Limine
# ------------------------------------------------------------

section "LIMINE"

if command -v limine >/dev/null 2>&1; then
    echo "Limine:"
    limine --version 2>&1 | head -n 5
else
    echo "[WARNING] Limine executable not found."
fi

echo
echo "Limine files in ISO:"
find iso -type f 2>/dev/null | grep -i limine

echo
echo "Limine request markers in current kernel sources:"
if grep -R -n -E 'LIMINE_REQUESTS_START_MARKER|LIMINE_REQUESTS_END_MARKER|LIMINE_.*_REQUEST' kernel boot 2>/dev/null; then
    :
else
    echo "No requests are defined yet."
fi

# ------------------------------------------------------------
# Linker
# ------------------------------------------------------------

section "LINKER SCRIPT"

if [ -f linker.ld ]; then

    echo "ENTRY:"
    grep -n 'ENTRY' linker.ld

    echo
    echo "Sections:"
    grep -n -E '^[[:space:]]*\.[a-zA-Z0-9_.]+' linker.ld

else
    echo "linker.ld not found."
fi

# ------------------------------------------------------------
# Build directory
# ------------------------------------------------------------

section "BUILD ARTIFACTS"

if [ -d build ]; then
    find build -maxdepth 2 -type f -printf '%p (%s bytes)\n' 2>/dev/null
else
    echo "build/ does not exist."
fi

# ------------------------------------------------------------
# Kernel ELF
# ------------------------------------------------------------

section "KERNEL ELF"

KERNEL_ELF="iso/boot/kernel.elf"

if [ -f "$KERNEL_ELF" ]; then

    echo "File:"
    ls -lh "$KERNEL_ELF"

    echo
    echo "ELF Header:"
    x86_64-elf-readelf -h "$KERNEL_ELF" 2>&1

    echo
    echo "Sections:"
    x86_64-elf-readelf -S "$KERNEL_ELF" 2>&1

    echo
    echo "Entry symbol:"
    x86_64-elf-readelf -s "$KERNEL_ELF" 2>/dev/null | grep -E '_start|kmain'

    echo
    echo "Limine sections:"
    x86_64-elf-readelf -S "$KERNEL_ELF" 2>/dev/null |
        grep -E 'limine|\.text|\.rodata|\.data|\.bss'

else
    echo "[MISSING] $KERNEL_ELF"
fi

# ------------------------------------------------------------
# ISO
# ------------------------------------------------------------

section "KERNEL ISO"

KERNEL_ISO="build/kernel.iso"

if [ -f "$KERNEL_ISO" ]; then

    echo "ISO:"
    ls -lh "$KERNEL_ISO"

    echo
    echo "File information:"
    file "$KERNEL_ISO"

    echo
    echo "ISO structure:"
    xorriso -indev "$KERNEL_ISO" -ls 2>&1 | head -n 100

else
    echo "[MISSING] $KERNEL_ISO"
fi

# ------------------------------------------------------------
# Boot configuration
# ------------------------------------------------------------

section "LIMINE CONFIG"

if [ -f iso/boot/limine.conf ]; then
    cat iso/boot/limine.conf
else
    echo "[MISSING] iso/boot/limine.conf"
fi

# ------------------------------------------------------------
# Bootloader files
# ------------------------------------------------------------

section "BOOTLOADER FILES"

for file in \
    iso/limine-bios-cd.bin \
    iso/limine-uefi-cd.bin \
    iso/limine-bios.sys \
    iso/BOOTX64.EFI \
    iso/BOOTIA32.EFI
do
    if [ -f "$file" ]; then
        ls -lh "$file"
        file "$file"
    else
        echo "[MISSING] $file"
    fi
done

# ------------------------------------------------------------
# QEMU
# ------------------------------------------------------------

section "QEMU"

if command -v qemu-system-x86_64 >/dev/null 2>&1; then

    qemu-system-x86_64 --version

    echo
    echo "QEMU binary:"
    command -v qemu-system-x86_64

else
    echo "[WARNING] QEMU not found."
fi

# ------------------------------------------------------------
# Toolchain architecture
# ------------------------------------------------------------

section "TOOLCHAIN TARGET"

if command -v x86_64-elf-gcc >/dev/null 2>&1; then
    echo "Target:"
    x86_64-elf-gcc -dumpmachine

    echo
    echo "Compiler:"
    x86_64-elf-gcc -v 2>&1 | tail -n 15
fi

# ------------------------------------------------------------
# Environment variables
# ------------------------------------------------------------

section "RELEVANT ENVIRONMENT VARIABLES"

env | grep -E \
    '^(PATH|SHELL|MSYSTEM|MSYS|MINGW|CC|CFLAGS|LDFLAGS|AS|LD|NASM|QEMU|XORRISO|LIMINE)=' |
    sort

# ------------------------------------------------------------
# Disk information
# ------------------------------------------------------------

section "PROJECT FILESYSTEM"

echo "Filesystem:"
df -h .

echo
echo "Project size:"
du -sh . 2>/dev/null

# ------------------------------------------------------------
# Summary
# ------------------------------------------------------------

section "SUMMARY"

echo "Diagnostic completed."
echo
echo "This script DOES NOT:"
echo "  - modify source files"
echo "  - modify Git"
echo "  - run make"
echo "  - delete build files"
echo "  - modify PATH"
echo "  - modify MSYS2"
echo
echo "============================================================"
echo "                    END OF DIAGNOSTIC"
echo "============================================================"
