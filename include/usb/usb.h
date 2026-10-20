#ifndef LUITY_USB_USB_H
#define LUITY_USB_USB_H

#include <stdbool.h>
#include <stdint.h>

enum usb_controller_kind {
    USB_CONTROLLER_NONE = 0,
    USB_CONTROLLER_UHCI,
};

struct usb_device_info {
    uint8_t address;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t device_class;
    uint8_t device_subclass;
    uint8_t device_protocol;
    uint8_t interface_number;
    uint8_t endpoint_in;
    uint16_t endpoint_max_packet;
    uint8_t endpoint0_max_packet;
    bool low_speed;
};

struct usb_setup_packet {
    uint8_t request_type;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    uint16_t length;
} __attribute__((packed));

/* Finds and starts a supported USB host controller and enumerates root ports. */
bool usb_init(void);

/* Called from the kernel's polling loop to consume completed input reports. */
void usb_poll(void);

enum usb_controller_kind usb_controller(void);
uint64_t usb_device_count(void);
bool usb_device_get(uint64_t index, struct usb_device_info *device);
bool usb_pointer_connected(void);
bool usb_control_transfer(const struct usb_device_info *device,
                          const struct usb_setup_packet *setup, void *data);
bool usb_interrupt_in(const struct usb_device_info *device,
                      uint8_t endpoint_address, uint8_t *data_toggle,
                      void *buffer, uint16_t capacity, uint16_t *actual_length);

#endif
