# Graphics and UI Subsystem

This document covers the framebuffer graphics layer and the visual shell used by the kernel.

## Overview

The graphics subsystem provides a minimal rendering API for a framebuffer surface. It supports:

- pixel plotting
- filled rectangles
- outlined rectangles
- 8x8 bitmap text
- scaled text rendering
- button widgets
- splash screen layout and state transitions

## Public graphics API

The main graphics declarations live in `include/graphics.h`.

Key structures:

- `struct gfx_color` — RGB color information
- `struct gfx_surface` — framebuffer metadata and pixel layout
- `struct gfx_context` — initialized graphics context bound to a surface

Key functions:

- `gfx_init()` — initialize a graphics context
- `gfx_clear()` — clear the framebuffer with a color
- `gfx_put_pixel()` — set one pixel
- `gfx_fill_rect()` — fill a rectangular area
- `gfx_draw_rect()` — draw a border rectangle
- `gfx_draw_char()` — render a single glyph
- `gfx_draw_text()` — render a string
- `gfx_draw_text_scaled()` — render a line of text at a custom scale

## Framebuffer model

The graphics system treats the display as a linear memory buffer with width, height, pitch, and pixel format metadata. It supports the RGB format used by the Limine framebuffer.

This abstraction makes the low-level display backend independent from the high-level rendering logic.

## Font rendering

The font data is stored in a compact bitmap format and rendered with a simple 8x8 glyph table. The renderer supports printable ASCII and falls back to a question mark for unsupported bytes.

The font handling is intentionally lightweight to keep the kernel minimal and predictable.

## Widgets

The kernel includes a simple widget system under `kernel/graphics/widgets/`.

The primary widget currently used is the button widget, which tracks:

- position
- size
- label
- state
- activation logic
- hover and press behavior

This makes the splash screen interactive even in a bare-metal environment.

## Splash screen

The UI splash screen is implemented in `kernel/ui/splash.c` and is rendered as a welcome screen with:

- a polished dark gradient background
- a top brand strip
- title text and subtitle text
- clickable buttons
- status information for the mouse device
- feature cards describing the kernel platform

The splash screen uses a small event-driven interaction model:

- mouse movement updates the cursor
- hover states change button appearance
- button press events trigger an action
- status messages are drawn at the bottom of the screen

## Cursor rendering

The mouse cursor is drawn in `kernel/drivers/input/mouse/cursor.c`.

The cursor implementation:

- stores the previous background pixels under the cursor
- erases the old cursor position before redrawing
- redraws the arrow using scaled bitmap pixels
- adds a shadow effect for readability
- remains synchronized with mouse events

## Notes for future expansion

The graphics layer is already capable of serving as a foundation for:

- window management
- a text console
- UI panels
- status overlays
- desktop-style components

A future improvement would be to separate rendering primitives from the framebuffer layer in a more formal driver model.

## Summary

The graphics and UI subsystem is one of the strongest parts of the project. It demonstrates a practical framebuffer API, text rendering, mouse cursor integration, and an interactive splash screen without depending on a complex operating environment.
