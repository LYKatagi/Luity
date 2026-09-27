# Kernel Architecture

This document explains the architecture of the Luity kernel and the responsibilities of the main subsystems.

## High-level overview

Luity follows a small, monolithic kernel design tuned for learning. The project compiles a freestanding x86_64 kernel image and boots it using the Limine bootloader.

The current architecture separates responsibilities into the following areas:

- boot code
- kernel entry point
- interrupt controller setup
- graphics subsystem
- input handling
- event queue
- UI/splash rendering

## Boot flow

The boot process begins with the assembly boot routines under `boot/`, which configure the environment needed by the kernel. After the bootloader transfers control, the kernel entry point starts from `kernel/main.c` and calls `kernel_start()`.

The startup sequence is roughly:

1. Initialize interrupt descriptor table
2. Initialize PIC
3. Register mouse IRQ handler
4. Enable PS/2 auxiliary port if available
5. Install PIT timer
6. Initialize framebuffer
7. Set up splash screen and UI state
8. Enable interrupts
9. Enter the main event loop

## Kernel startup

The main kernel entry is defined in `kernel/main.c`.

```c
#include <kernel.h>

void kmain(void)
{
    kernel_start();
}
```

`kernel_start()` is the actual core routine. This function initializes every major subsystem and then enters the event loop.

## Runtime model

The kernel executes in ring 0, with direct access to hardware. There is no user-mode process scheduler yet. Most kernel logic is synchronous and event-driven.

The current runtime model includes:

- interrupt-driven input handling
- polling-based checks for some hardware state
- a small event queue for mouse movement and button transitions
- a graphical splash screen that reacts to events

## Interrupt and timer model

### IDT

The interrupt descriptor table is initialized in the IDT subsystem. This is required before any interrupt-driven behavior can work reliably.

### PIC

The PIC initialization configures legacy programmable interrupt controllers so the system can route interrupts correctly.

### PIT

The PIT is configured to provide periodic ticks at a fixed frequency. This is useful for timing and fallback synchronization patterns.

### IRQ12 and PS/2 mouse

The PS/2 auxiliary port is initialized and the IRQ12 handler is registered. Mouse bytes are processed as they arrive and converted into kernel events.

## Graphics model

The kernel controls a framebuffer surface exposed through `gfx_context`. Rendering is done through pixel APIs and text drawing utilities.

The graphics pipeline is organized as:

- framebuffer initialization
- surface setup
- drawing primitives
- glyph rendering
- UI widgets and splash screen on top

## Event model

Kernel events are represented by a simple structure that carries:

- event type
- position information
- button state
- associated metadata

The event queue stores incoming data and provides a polling interface used by the kernel main loop.

## Current architectural limitations

This kernel is intentionally minimal and has several important limitations:

- no proper memory manager
- no virtual memory
- no process scheduler
- no filesystem
- no multi-user environment
- no robust driver abstraction layer

These gaps are expected in an early-stage educational kernel but should be addressed as the project evolves.

## Design direction

The project is positioned as a learning-first kernel. The next likely architectural steps are:

1. introduce a memory manager
2. isolate device interfaces behind cleaner abstractions
3. expand interrupt handling and error recovery
4. add more kernel events and input types
5. build a basic userland and scheduler

## Summary

Luity is a compact, monolithic x86_64 kernel focused on understanding the fundamentals of OS development. It already demonstrates several real concepts: interrupt handling, graphics, PS/2 input, event queues, and UI rendering. The architecture remains intentionally simple so that each subsystem is easy to understand and extend.
