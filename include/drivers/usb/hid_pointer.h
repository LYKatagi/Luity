#ifndef LUITY_DRIVERS_USB_HID_POINTER_H
#define LUITY_DRIVERS_USB_HID_POINTER_H

#include <stdbool.h>
#include <stdint.h>

enum usb_hid_pointer_kind {
    USB_HID_POINTER_NONE = 0,
    USB_HID_POINTER_BOOT_MOUSE,
    USB_HID_POINTER_RELATIVE_MOUSE,
    USB_HID_POINTER_TOUCHPAD,
};

bool usb_hid_pointer_init(bool boot_mouse, const uint8_t *descriptor,
                          uint16_t descriptor_length);
enum usb_hid_pointer_kind usb_hid_pointer_kind(void);
bool usb_hid_pointer_report(const uint8_t *report, uint16_t length);

#endif
