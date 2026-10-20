#ifndef LUITY_DRIVERS_USB_UHCI_H
#define LUITY_DRIVERS_USB_UHCI_H

#include <stdbool.h>
#include <usb/usb.h>

bool uhci_init(void);
void uhci_poll(void);
bool uhci_pointer_connected(void);
bool uhci_device_get(uint64_t index, struct usb_device_info *device);
bool uhci_control_transfer(const struct usb_device_info *device,
                           const struct usb_setup_packet *setup, void *data);
bool uhci_interrupt_in(const struct usb_device_info *device,
                       uint8_t endpoint_address, uint8_t *data_toggle,
                       void *buffer, uint16_t capacity,
                       uint16_t *actual_length);

#endif
