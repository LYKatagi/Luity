# Building Luity

This guide explains how to build, run, and debug the Luity kernel from source.

## Requirements

Before building the project, make sure the following tools are installed:

- GNU Make
- NASM
- x86_64 cross-compilation toolchain (`x86_64-elf-gcc`, `x86_64-elf-ld`, `x86_64-elf-as`)
- QEMU (`qemu-system-x86_64`)
- Limine utilities
- `xorriso`

## Project layout relevant to build

- `Makefile` — top-level build orchestration
- `linker.ld` — kernel ELF layout
- `boot/` — assembly boot code
- `kernel/` — C kernel source files
- `include/` — public headers
- `iso/` — bootloader and image assets
- `build/` — generated intermediate objects and outputs

## Build commands

From the project root:

```bash
make
```

This should compile the kernel and generate the bootable ISO image.

### Outputs

The build generally produces:

- `iso/boot/kernel.elf`
- `build/kernel.iso`

## Running the kernel

To launch the generated ISO in QEMU:

```bash
make run
```

This command uses QEMU with a virtual CD-ROM attached to the generated ISO image.

## Clean build

To remove generated files and rebuild from scratch:

```bash
make clean
make
```

## Troubleshooting

### Missing cross-toolchain

If compilation fails with `x86_64-elf-gcc: not found`, install the cross toolchain and ensure it is on the `PATH`.

### Limine not found

Make sure the Limine binaries are installed and accessible. The project expects the `limine` executable during ISO generation.

### QEMU launch issues

Verify that QEMU is installed and supports x86_64 system emulation. If the ISO fails to boot, ensure the generated image exists and the build completed without errors.

### Assembly failure

Check whether NASM is installed and whether the assembler target matches the project configuration.

## Recommended validation flow

Use the following sequence for reliable debugging:

```bash
make clean
make
make run
```

If the build fails, fix the compile error before attempting to run the system.

## Future enhancements

Later stages of the project may add:

- automatic dependency generation
- CI validation scripts
- build profiles for debug and release modes
- stricter compiler sanitization options
- cross-platform scripts for Linux/macOS/Windows hosts
