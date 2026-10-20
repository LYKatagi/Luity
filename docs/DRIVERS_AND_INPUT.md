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
- reads 3-byte relative mouse packets or 6-byte Synaptics touchpad packets
- decodes movement, touch contacts, and button state
- clamps the cursor position within the framebuffer bounds
- posts mouse events to the kernel event queue

The PS/2 driver also probes for Synaptics touchpads. When the device identifies
itself with the Synaptics PS/2 signature, the driver switches it into absolute
mode, reads its model to select the six-byte packet layout, maps coordinates to
the framebuffer, and converts a short single-finger contact into a left click.
Other devices continue through the standard three-byte relative mouse path,
which also covers touchpads exposed by firmware in PS/2 mouse compatibility
mode.

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

## USB host and pointing devices

The USB core API is in `include/usb/usb.h` and `kernel/usb/usb.c`. It exposes
device information, control transfers, interrupt-IN transfers, and a polling
hook for class drivers. UHCI host-controller support lives in
`kernel/drivers/usb/uhci.c`; it discovers a PCI UHCI controller, resets its
root ports, and uses DMA memory allocated from Limine's usable-memory map via
the HHDM.

The HID driver accepts a USB boot mouse or a simple report-protocol pointer
connected directly to either UHCI root port. It parses relative X/Y mouse
reports and absolute X/Y touchpad reports with Tip Switch or Contact Count.
Touchpad coordinates are mapped to the framebuffer and a short tap generates a
left click. Synaptics PS/2 touchpads use native absolute mode when detected;
other PS/2 touchpads work when firmware exposes them in standard mouse
compatibility mode. All reports use the shared pointer state/event path, so
they reach the existing buttons and graphics.

Current limits: UHCI (USB 1.1) only, one directly attached USB pointer, and no
USB hubs, OHCI/EHCI/xHCI controllers, or I2C-HID touchpads. The report-protocol
HID parser and the Synaptics path support single-contact cursor motion and do
not implement multi-finger gestures. The `make run` target attaches a QEMU UHCI
controller and a virtual USB mouse.

The code is a good base for later expansion into a more complete hardware abstraction layer.

## Summary

The driver and input layer is a good example of practical kernel engineering. It covers device initialization, IRQ handling, packet decoding, and event-driven UI updates in a simple but teachable fashion.
