# Project Structure

This document describes the repository layout and the purpose of each major folder.

## Root level

- `Makefile` — build orchestration for the kernel and ISO image
- `README.md` — project-level description and quick start
- `linker.ld` — ELF linker script
- `diagnostic.sh` — helper script for diagnostics
- `tools/` — project utilities and automation scripts
- `boot/` — assembly boot code and startup routines
- `include/` — public headers and kernel API declarations
- `kernel/` — kernel implementation source files
- `iso/` — bootloader and Limine assets
- `docs/` — project documentation

## `boot/`

Contains the assembly boot code used before the C kernel takes over.

This area is where the system begins to establish the required environment for a minimal kernel runtime.

## `include/`

Public headers shared across the kernel.

Examples include:

- graphics API
- kernel entry declarations
- Limine-related definitions
- driver and UI interface declarations

## `kernel/`

This is the main implementation tree.

### `kernel/core/`

Contains the core startup logic and the main kernel runtime flow.

### `kernel/drivers/`

Low-level hardware drivers, including:

- PS/2 controller interface
- PS/2 relative mouse and Synaptics touchpad support
- USB HID pointing devices and UHCI host-controller support
- other future device abstractions

### `kernel/boot/`

Limine framebuffer setup and boot-time DMA memory allocation.

### `kernel/usb/`

Generic USB device and transfer API, implemented over the available host
controller drivers.

### `kernel/events/`

Event queue and event definitions used to pass hardware and user interactions into the application logic.

### `kernel/graphics/`

Rendering pipeline, font data, and drawing primitives.

### `kernel/ui/`

User-facing interface code, including the splash screen and early UI states.

### `kernel/interrupts/`

Interrupt controller, IDT, IRQ, PIC, and timer infrastructure.

## `iso/`

This folder stores the boot image assets and Limine configuration files used to build the final ISO.

## `build/`

Generated build output is placed here during compilation. This includes object files and the final image artifacts.

## `docs/`

Developer-facing documentation for architecture, build flow, graphics, and hardware drivers.

## Extending the project

When adding new subsystems, follow the same separation pattern:

- public interface in `include/`
- implementation in `kernel/`
- boot-related code in `boot/`
- generated artifacts in `build/`
- product assets in `iso/`

This makes the source tree easier to understand and reduces the chance of mixing hardware logic with user-facing code.

## Summary

The repository is intentionally organized into a small number of clean areas, which makes the project easier to teach and easier to expand as the kernel evolves.
