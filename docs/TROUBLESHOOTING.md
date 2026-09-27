# Troubleshooting

This document captures the most common kernel build and runtime problems and the usual fixes.

## Build issues

### Toolchain not found

Symptoms:

- `x86_64-elf-gcc: command not found`
- `x86_64-elf-ld: command not found`

Fix:

- install the x86_64 cross toolchain
- confirm the binaries are on your `PATH`
- reopen the terminal or source your shell profile

### NASM missing

Symptoms:

- `nasm: command not found`

Fix:

- install NASM
- verify that the assembler remains available from the project shell

### Limine missing

Symptoms:

- `limine: command not found`

Fix:

- install Limine
- ensure the relevant binaries are reachable in the current environment

## ISO creation issues

If the build reaches the ISO stage and fails, verify:

- `xorriso` is installed
- the `iso/` directory contains the required bootloader files
- the required Limine files are present and not corrupted

## Runtime issues

### QEMU does not boot the kernel

Check the following:

- the `build/kernel.iso` file exists
- the image was generated successfully
- the host environment supports virtualization and QEMU x86_64 emulation

### Black screen or no output

This usually indicates one of the following:

- framebuffer not initialized
- boot loader configuration is wrong
- kernel entry point did not execute
- interrupt initialization failed early

Use the boot output and logs to narrow down where execution stops.

### Mouse not detected

This can happen when:

- the PS/2 controller is not initialized correctly
- the auxiliary device is unavailable
- the IRQ is not being routed as expected

Check the initialization code and confirm the PS/2 mouse device is enabled before relying on the splash screen interactions.

## Debugging guidelines

### 1. Build only once

Run the full build and confirm the compile stage succeeds before worrying about runtime issues.

### 2. Check the lowest failing layer

If the kernel fails to initialize, inspect the earliest subsystem in the startup flow first.

### 3. Validate assumptions

On bare-metal systems, many issues are caused by incorrect register assumptions, wrong bit masks, or wrong interrupt configuration.

### 4. Keep logs simple

Add targeted debug output only to the minimal subsystem you are debugging, then remove it once the issue is resolved.

## Useful commands

```bash
make clean
make
make run
```

For more targeted verification, inspect the generated files and make sure the ELF and ISO are being regenerated after code changes.

## Future improvements

A stronger debugging story would include:

- serial logging support
- a debug console
- build-time configuration profiles
- CI validation scripts
- a minimal hardware test harness

## Summary

Most problems in a bare-metal kernel are caused by toolchain configuration, boot assumptions, or low-level hardware initialization. The project is still educational, so debugging should be done in a layered, methodical way.
