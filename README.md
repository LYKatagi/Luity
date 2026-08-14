# Luity

**Luity** is a hobby operating system kernel written from scratch in C and Assembly.

The project is being developed as a learning experience focused on understanding how operating systems work at a low level.

## Goals

* Build a kernel from scratch
* Learn x86_64 architecture
* Understand bootloaders and the boot process
* Implement framebuffer graphics
* Develop basic kernel facilities
* Experiment with memory management
* Eventually build a useful environment for programming

## Architecture

* **Target:** x86_64
* **Language:** C
* **Assembly:** NASM
* **Bootloader:** Limine
* **Cross-compiler:** `x86_64-elf-gcc`
* **Linker:** `x86_64-elf-ld`
* **Emulator:** QEMU

## Project Status

Luity is currently in early development.

The initial objective is to establish a reliable boot process and gradually build the kernel from there.

## Building

The project uses a cross-compilation toolchain targeting `x86_64-elf`.

Once the required toolchain is installed, the kernel can be built using:

```bash
make
```

The generated kernel and bootable image are placed in the build directory.

## Running

Luity is currently intended to be tested with QEMU.

```bash
make run
```

> Build and run commands may change as development progresses.

## Repository Structure

```text
Luity/
├── boot/          # Boot-related files
├── include/       # Kernel headers
├── kernel/        # Kernel source code
├── build/         # Build output
├── iso/           # Bootloader files
├── linker.ld      # Kernel linker script
├── Makefile       # Build system
└── README.md
```

## License

This project is currently under development. See the `LICENSE` file for the applicable license.

## Disclaimer

Luity is an experimental hobby project and is **not intended for production use**.
