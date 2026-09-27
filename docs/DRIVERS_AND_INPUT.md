# Drivers and Input Processing

This section documents the parts of the kernel responsible for hardware interfaces and user input.

## Overview

The input system is built around PS/2 devices and interrupt-driven event generation. The project currently focuses on mouse support, but the same patterns are reusable for keyboard and other peripherals.

## PS/2 controller

The PS/2 controller code lives under `kernel/drivers/input/ps2/` and handles low-level register access, data transfers, and command acknowledgment.

Important responsibilities include:

- reading controller status
- writing data to the controller
- enabling the auxiliary port
- reading mouse bytes
- handling command acknowledgment and timeouts

## Mouse driver

The mouse driver is implemented in `kernel/drivers/input/mouse/mouse.c`.

It performs the following:

- initializes the PS/2 mouse
- restores default configuration
- enables reporting mode
- reads a 3-byte packet
- decodes relative movement and button state
- clamps the cursor position within the framebuffer bounds
- posts mouse events to the kernel event queue

## Mouse packet format

PS/2 mice send 3-byte packets containing:

- packet flags
- relative X movement
- relative Y movement

The code interprets the button bits and converts delta movement into absolute cursor coordinates.

## Event generation

When the cursor moves or a button changes state, the mouse driver posts a `kernel_event` to the internal event queue.

The current event types include:

- mouse move
- mouse button down
- mouse button up

This allows the kernel UI and splash screen to respond to user interactions in a centralized way.

## Interrupt handling

The mouse input path uses IRQ12, which is associated with the PS/2 auxiliary device.

The startup sequence registers a handler with:

- `irq12_register_handler(mouse_handle_byte)`

The IRQ handler receives each byte from the device and feeds it into the packet decoder.

## Timer fallback

The PIT is also used as a reliability fallback. The kernel initializes a 100 Hz timer and allows periodic checks to recover when interrupts are unreliable or delayed.

## Input flow

The typical flow is:

1. PS/2 device sends packet bytes
2. IRQ12 receives the bytes
3. driver accumulates the 3-byte mouse packet
4. state is updated with new movement and buttons
5. event is posted to the kernel queue
6. main loop consumes the event and updates the cursor/UI

## Limitations and improvements

The current mouse implementation is intentionally basic:

- no keyboard support yet
- no advanced pointer acceleration
- no multi-device abstraction
- no generalized input framework

The code is a good base for later expansion into a more complete hardware abstraction layer.

## Summary

The driver and input layer is a good example of practical kernel engineering. It covers device initialization, IRQ handling, packet decoding, and event-driven UI updates in a simple but teachable fashion.
