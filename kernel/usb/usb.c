#include <drivers/usb/uhci.h>
#include <usb/usb.h>

static enum usb_controller_kind active_controller;

bool usb_init(void)
{
    active_controller = USB_CONTROLLER_NONE;
    if (!uhci_init()) {
        return false;
    }
    active_controller = USB_CONTROLLER_UHCI;
    return true;
}

void usb_poll(void)
{
    if (active_controller == USB_CONTROLLER_UHCI) {
        uhci_poll();
    }
}

enum usb_controller_kind usb_controller(void)
{
    return active_controller;
}

uint64_t usb_device_count(void)
{
    struct usb_device_info device;
    return active_controller != USB_CONTROLLER_NONE &&
                   uhci_device_get(0, &device)
               ? 1
               : 0;
}

bool usb_device_get(uint64_t index, struct usb_device_info *device)
{
    return active_controller == USB_CONTROLLER_UHCI &&
           uhci_device_get(index, device);
}

bool usb_pointer_connected(void)
{
    return active_controller == USB_CONTROLLER_UHCI &&
           uhci_pointer_connected();
}

bool usb_control_transfer(const struct usb_device_info *device,
                          const struct usb_setup_packet *setup, void *data)
{
    return active_controller == USB_CONTROLLER_UHCI &&
           uhci_control_transfer(device, setup, data);
}

bool usb_interrupt_in(const struct usb_device_info *device,
                      uint8_t endpoint_address, uint8_t *data_toggle,
                      void *buffer, uint16_t capacity,
                      uint16_t *actual_length)
{
    return active_controller == USB_CONTROLLER_UHCI &&
           uhci_interrupt_in(device, endpoint_address, data_toggle,
                             buffer, capacity, actual_length);
}
