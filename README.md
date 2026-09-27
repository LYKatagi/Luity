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

The kernel ELF is written to iso/boot/kernel.elf, and the bootable image is
written to build/kernel.iso.

## Running

Luity is currently intended to be tested with QEMU.

```bash
make run
```

> Build and run commands may change as development progresses.

## Graphics API

The public API is declared in include/graphics.h. It provides pixel drawing,
filled and outlined rectangles, 8x8 characters, and ASCII text. The Limine
adapter in kernel/boot/limine_framebuffer.c initializes a graphics context;
the drawing routines live in kernel/graphics/graphics.c, and the glyph data
lives in kernel/graphics/font8x8.c. Interactive widgets are in
kernel/graphics/widgets/ and the welcome screen is in kernel/ui/.

Text wraps at the right edge and supports newlines and four-character tabs.
Passing a null background keeps unset glyph pixels transparent. Printable
ASCII characters are supported; other bytes are drawn as question marks. The
API also provides scaled bitmap text for headings and labels.

```c
#include <graphics.h>
#include <limine_framebuffer.h>

struct gfx_context screen;
struct gfx_color white = {255, 255, 255};

if (limine_framebuffer_init(&screen)) {
    gfx_clear(&screen, (struct gfx_color){18, 24, 38});
    gfx_draw_text(&screen, 16, 16, "Hello, Luity!", white, 0);
}
```

## Mouse input

The kernel initializes the legacy PIC and the PS/2 auxiliary port, then routes
IRQ12 to the mouse packet handler. A 100 Hz PIT tick also checks the controller
buffer as a fallback for platforms that fail to deliver the auxiliary IRQ.
The cursor is rendered through the graphics API, starts in the center of the
framebuffer, and restores the pixels beneath its previous position as it moves.
Mouse movement and button transitions are queued as kernel events. The welcome
screen has clickable **Get Started** and **About** buttons with hover and pressed
states. The cursor uses a scaled black and white arrow with a drop shadow.

## Repository Structure

```text
Luity/
├── boot/          # Boot-related files
├── include/       # Graphics, font, and Limine headers
├── kernel/        # Kernel source code
├── build/         # Build output
├── iso/           # Bootloader files
├── linker.ld      # Kernel linker script
├── Makefile       # Build system
└── README.md
```

## License

No license file is included, so the project has not declared a license yet.

## Disclaimer

Luity is an experimental hobby project and is **not intended for production use**.
