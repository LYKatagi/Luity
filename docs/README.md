# Luity Documentation

This directory contains the main developer documentation for the Luity kernel project.

## Documentation index

- [BUILDING.md](BUILDING.md) — build tools, prerequisites, and the full compile flow
- [ARCHITECTURE.md](ARCHITECTURE.md) — system architecture, boot flow, and runtime model
- [GRAPHICS_AND_UI.md](GRAPHICS_AND_UI.md) — framebuffer graphics, drawing primitives, and splash screen UI
- [DRIVERS_AND_INPUT.md](DRIVERS_AND_INPUT.md) — PS/2, mouse handling, IRQs, and event flow
- [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md) — repository layout and responsibilities per directory
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) — common issues and debugging tips

## Project overview

Luity is a hobby operating system kernel written in C and assembly for x86_64 systems. The project focuses on learning low-level OS development: bootstrapping, interrupt handling, graphics, input devices, and kernel event processing.

The current codebase is intentionally educational and experimental. It is not meant for production use, but it serves as a practical reference for understanding how a minimal kernel is built from scratch.

## Development goals

- Learn the x86_64 boot and execution model
- Understand Limine boot protocol and ELF loading
- Build a simple framebuffer graphics API
- Integrate PS/2 mouse input and interrupt-driven event handling
- Explore basic kernel event dispatch patterns
- Create a clean, understandable project structure for future expansion

## Typical workflow

1. Install the required toolchain.
2. Build the kernel with the Makefile.
3. Run the generated ISO in QEMU.
4. Use the project documentation to understand each subsystem.
5. Extend modules one by one without breaking the boot path.

## Recommended reading order

If you are new to the project, read the docs in this order:

1. [BUILDING.md](BUILDING.md)
2. [ARCHITECTURE.md](ARCHITECTURE.md)
3. [GRAPHICS_AND_UI.md](GRAPHICS_AND_UI.md)
4. [DRIVERS_AND_INPUT.md](DRIVERS_AND_INPUT.md)
5. [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)

## Notes

- The project targets bare-metal x86_64 execution.
- Most code assumes a freestanding environment and does not rely on libc.
- Display output is handled through a framebuffer abstraction.
- Mouse input is implemented through PS/2 controller logic and IRQ dispatch.

This documentation is intentionally focused on clarity and maintainability so the project can be expanded safely as the kernel grows.
