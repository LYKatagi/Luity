# Changelog

All notable changes to Luity will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-08-16

### Added

- Initial Luity kernel structure.
- x86_64 kernel entry point.
- Initial kernel stack.
- Basic `kmain` kernel function.
- Limine 12.5.2 bootloader integration.
- BIOS and UEFI boot support.
- Bootable kernel ISO.
- Initial Makefile build system.
- Initial linker script.
- Initial Git configuration.

### Fixed

- Corrected the Limine BIOS CD boot image.
- Fixed the boot process so Luity can successfully boot in QEMU.

### Infrastructure

- Added Limine boot files to `iso/`.
- Added cross-compilation support using `x86_64-elf-gcc`.
- Added NASM assembly support.
- Added QEMU testing support.