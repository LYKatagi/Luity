#include <boot/dma.h>
#include <drivers/usb/hid_pointer.h>
#include <drivers/usb/uhci.h>
#include <interrupts/pit.h>
#include <io.h>

#define PCI_CONFIG_ADDRESS 0x0CF8
#define PCI_CONFIG_DATA    0x0CFC
#define UHCI_CLASS         0x0C
#define UHCI_SUBCLASS      0x03
#define UHCI_PROGIF        0x00

#define UHCI_REG_COMMAND   0x00
#define UHCI_REG_STATUS    0x02
#define UHCI_REG_INTERRUPT 0x04
#define UHCI_REG_FRAME     0x06
#define UHCI_REG_FRAMEBASE 0x08
#define UHCI_REG_SOF       0x0C
#define UHCI_REG_PORT1     0x10
#define UHCI_REG_CONFIG    0x40

#define UHCI_CMD_RUN       (1u << 0)
#define UHCI_CMD_HCRESET   (1u << 1)
#define UHCI_CMD_GRESET    (1u << 2)
#define UHCI_CMD_CONFIGURE (1u << 6)
#define UHCI_CMD_MAX64     (1u << 7)

#define UHCI_PORT_CONNECT  (1u << 0)
#define UHCI_PORT_CSC      (1u << 1)
#define UHCI_PORT_ENABLE   (1u << 2)
#define UHCI_PORT_PEC      (1u << 3)
#define UHCI_PORT_LOW      (1u << 8)
#define UHCI_PORT_RESET    (1u << 9)
#define UHCI_LINK_TERMINATE 1u
#define UHCI_LINK_DEPTH     4u

#define UHCI_TD_ACTIVE      (1u << 23)
#define UHCI_TD_IOC         (1u << 24)
#define UHCI_TD_LOW_SPEED   (1u << 26)
#define UHCI_TD_ERRORS      0x00760000u
#define UHCI_TD_ERROR_COUNT (3u << 27)

#define USB_PID_OUT   0xE1u
#define USB_PID_IN    0x69u
#define USB_PID_SETUP 0x2Du
#define USB_REQ_GET_DESCRIPTOR 6u
#define USB_REQ_SET_ADDRESS    5u
#define USB_REQ_SET_CONFIG     9u
#define USB_REQ_SET_PROTOCOL   0x0Bu
#define USB_DESC_DEVICE        1u
#define USB_DESC_CONFIGURATION 2u
#define USB_DESC_HID            0x21u
#define USB_DESC_HID_REPORT     0x22u

#define USB_CONTROL_BUFFER_SIZE 4096u
#define USB_CONTROL_TD_COUNT    128u
#define USB_MAX_CONFIG_SIZE     512u
#define USB_ROOT_PORTS          2u

struct uhci_td {
    uint32_t link;
    volatile uint32_t status;
    uint32_t token;
    uint32_t buffer;
} __attribute__((packed, aligned(16)));

struct uhci_host {
    uint16_t io_base;
    uint32_t frame_list_physical;
    uint32_t td_pool_physical;
    uint32_t control_buffer_physical;
    uint32_t mouse_buffer_physical;
    uint32_t mouse_td_physical;
    uint32_t *frame_list;
    struct uhci_td *td_pool;
    uint8_t *control_buffer;
    uint8_t *mouse_buffer;
    struct uhci_td *mouse_td;
    uint8_t mouse_toggle;
    uint8_t port_number;
    bool low_speed;
    bool initialized;
    bool pointer_ready;
    struct usb_device_info device;
};

static struct uhci_host host;

static uint32_t pci_config_read(uint8_t bus, uint8_t device,
                                uint8_t function, uint8_t offset)
{
    uint32_t address = 0x80000000u | ((uint32_t)bus << 16) |
                       ((uint32_t)device << 11) |
                       ((uint32_t)function << 8) | (offset & 0xFCu);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

static void pci_config_write(uint8_t bus, uint8_t device,
                             uint8_t function, uint8_t offset,
                             uint32_t value)
{
    uint32_t address = 0x80000000u | ((uint32_t)bus << 16) |
                       ((uint32_t)device << 11) |
                       ((uint32_t)function << 8) | (offset & 0xFCu);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

static bool pci_find_uhci(uint16_t *io_base)
{
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            uint32_t header = pci_config_read((uint8_t)bus, device, 0, 0x0C);
            uint16_t vendor = (uint16_t)pci_config_read(
                (uint8_t)bus, device, 0, 0x00);
            if (vendor == 0xFFFFu) {
                continue;
            }

            uint8_t functions = ((header >> 16) & 0x80u) != 0 ? 8 : 1;
            for (uint8_t function = 0; function < functions; function++) {
                uint32_t identity = pci_config_read(
                    (uint8_t)bus, device, function, 0x00);
                if ((uint16_t)identity == 0xFFFFu) {
                    continue;
                }

                uint32_t class_info = pci_config_read(
                    (uint8_t)bus, device, function, 0x08);
                if ((uint8_t)(class_info >> 24) != UHCI_CLASS ||
                    (uint8_t)(class_info >> 16) != UHCI_SUBCLASS ||
                    (uint8_t)(class_info >> 8) != UHCI_PROGIF) {
                    continue;
                }

                uint32_t bar0 = pci_config_read(
                    (uint8_t)bus, device, function, 0x10);
                if ((bar0 & 1u) == 0 || (bar0 & 0xFFFCu) == 0) {
                    continue;
                }

                uint32_t command = pci_config_read(
                    (uint8_t)bus, device, function, 0x04);
                command |= 0x00000005u; /* I/O space and bus mastering. */
                pci_config_write((uint8_t)bus, device, function, 0x04,
                                 command);
                *io_base = (uint16_t)(bar0 & 0xFFFCu);
                return true;
            }
        }
    }
    return false;
}

static uint16_t uhci_read16(uint16_t offset)
{
    return inw((uint16_t)(host.io_base + offset));
}

static void uhci_write16(uint16_t offset, uint16_t value)
{
    outw((uint16_t)(host.io_base + offset), value);
}

static void uhci_write32(uint16_t offset, uint32_t value)
{
    outl((uint16_t)(host.io_base + offset), value);
}

static void clear_frame_list(void)
{
    for (uint32_t i = 0; i < 1024; i++) {
        host.frame_list[i] = UHCI_LINK_TERMINATE;
    }
    __asm__ volatile("" : : : "memory");
}

static void schedule_all_frames(uint32_t physical_td)
{
    uint32_t link = physical_td | UHCI_LINK_DEPTH;
    for (uint32_t i = 0; i < 1024; i++) {
        host.frame_list[i] = link;
    }
    __asm__ volatile("" : : : "memory");
}

static bool uhci_controller_start(void)
{
    if (!pci_find_uhci(&host.io_base)) {
        return false;
    }

    host.frame_list = boot_dma_alloc(4096, 4096,
                                     &host.frame_list_physical);
    host.td_pool = boot_dma_alloc(4096, 4096, &host.td_pool_physical);
    host.control_buffer = boot_dma_alloc(USB_CONTROL_BUFFER_SIZE, 4096,
                                         &host.control_buffer_physical);
    host.mouse_buffer = boot_dma_alloc(4096, 4096,
                                       &host.mouse_buffer_physical);
    if (host.frame_list == 0 || host.td_pool == 0 ||
        host.control_buffer == 0 || host.mouse_buffer == 0) {
        return false;
    }

    clear_frame_list();
    uhci_write16(UHCI_REG_COMMAND, UHCI_CMD_GRESET);
    pit_delay_ms(50);
    uhci_write16(UHCI_REG_COMMAND, 0);
    pit_delay_ms(10);
    uhci_write16(UHCI_REG_COMMAND, UHCI_CMD_HCRESET);
    bool reset_complete = false;
    for (uint32_t i = 0; i < 100000; i++) {
        if ((uhci_read16(UHCI_REG_COMMAND) & UHCI_CMD_HCRESET) == 0) {
            reset_complete = true;
            break;
        }
    }
    if (!reset_complete) {
        return false;
    }

    uhci_write16(UHCI_REG_STATUS, 0x003Fu);
    uhci_write16(UHCI_REG_INTERRUPT, 0);
    uhci_write16(UHCI_REG_FRAME, 0);
    uhci_write32(UHCI_REG_FRAMEBASE, host.frame_list_physical);
    outb((uint16_t)(host.io_base + UHCI_REG_SOF), 0x40);
    uhci_write32(UHCI_REG_CONFIG, 1);
    uhci_write16(UHCI_REG_COMMAND,
                 UHCI_CMD_RUN | UHCI_CMD_CONFIGURE | UHCI_CMD_MAX64);
    host.initialized = true;
    return true;
}

static bool uhci_reset_port(uint8_t port_number)
{
    uint16_t reg = (uint16_t)(UHCI_REG_PORT1 + port_number * 2u);
    uint16_t value = uhci_read16(reg);
    if ((value & UHCI_PORT_CONNECT) == 0) {
        return false;
    }

    host.low_speed = (value & UHCI_PORT_LOW) != 0;
    value &= (uint16_t)~(UHCI_PORT_CSC | UHCI_PORT_PEC);
    uhci_write16(reg, (uint16_t)(value | UHCI_PORT_RESET));
    pit_delay_ms(50);

    value = uhci_read16(reg);
    value &= (uint16_t)~(UHCI_PORT_RESET | UHCI_PORT_CSC | UHCI_PORT_PEC);
    uhci_write16(reg, (uint16_t)(value | UHCI_PORT_CSC | UHCI_PORT_PEC));
    pit_delay_ms(10);

    value = uhci_read16(reg);
    host.low_speed = (value & UHCI_PORT_LOW) != 0;
    return (value & UHCI_PORT_CONNECT) != 0 &&
           (value & UHCI_PORT_ENABLE) != 0;
}

static uint32_t uhci_token(uint8_t pid, uint8_t address, uint8_t endpoint,
                           uint8_t toggle, uint16_t length)
{
    uint16_t encoded_length = length == 0 ? 0x07FFu : (uint16_t)(length - 1);
    return pid | ((uint32_t)address << 8) | ((uint32_t)endpoint << 15) |
           ((uint32_t)(toggle & 1u) << 19) |
           ((uint32_t)encoded_length << 21);
}

static uint32_t uhci_td_status(bool low_speed, bool interrupt_on_complete)
{
    return UHCI_TD_ACTIVE | UHCI_TD_ERROR_COUNT |
           (low_speed ? UHCI_TD_LOW_SPEED : 0) |
           (interrupt_on_complete ? UHCI_TD_IOC : 0);
}

static uint16_t uhci_actual_length(uint32_t status)
{
    uint16_t encoded = (uint16_t)(status & 0x7FFu);
    return encoded == 0x7FFu ? 0 : (uint16_t)(encoded + 1u);
}

static bool uhci_wait_transfer(uint32_t count)
{
    uint64_t deadline = pit_ticks() + 200;
    for (;;) {
        bool active = false;
        for (uint32_t i = 0; i < count; i++) {
            uint32_t status = host.td_pool[i].status;
            if ((status & UHCI_TD_ACTIVE) != 0) {
                active = true;
                continue;
            }
            if ((status & UHCI_TD_ERRORS) != 0) {
                clear_frame_list();
                return false;
            }
        }
        if (!active) {
            clear_frame_list();
            return true;
        }
        if (pit_ticks() >= deadline) {
            clear_frame_list();
            return false;
        }
        __asm__ volatile("hlt" : : : "memory");
    }
}

static bool uhci_control(uint8_t address, uint8_t max_packet,
                         bool low_speed, const struct usb_setup_packet *setup,
                         void *data, uint16_t length)
{
    if (setup == 0 || max_packet == 0 || length > USB_MAX_CONFIG_SIZE ||
        (length != 0 && data == 0)) {
        return false;
    }

    uint8_t *transfer_data = host.control_buffer + 64;
    if ((setup->request_type & 0x80u) == 0 && length != 0) {
        const uint8_t *source = data;
        for (uint16_t i = 0; i < length; i++) {
            transfer_data[i] = source[i];
        }
    }
    const uint8_t *setup_bytes = (const uint8_t *)setup;
    for (uint32_t i = 0; i < sizeof(*setup); i++) {
        host.control_buffer[i] = setup_bytes[i];
    }

    uint32_t td_count = 0;
    uint8_t setup_toggle = 0;
    host.td_pool[td_count] = (struct uhci_td) {
        .link = 0,
        .status = uhci_td_status(low_speed, false),
        .token = uhci_token(USB_PID_SETUP, address, 0, setup_toggle, 8),
        .buffer = host.control_buffer_physical,
    };
    td_count++;

    uint16_t offset = 0;
    uint8_t toggle = 1;
    bool data_in = (setup->request_type & 0x80u) != 0;
    while (offset < length) {
        uint16_t packet_length = (uint16_t)(length - offset);
        if (packet_length > max_packet) {
            packet_length = max_packet;
        }
        if (td_count >= USB_CONTROL_TD_COUNT - 1) {
            clear_frame_list();
            return false;
        }

        host.td_pool[td_count] = (struct uhci_td) {
            .link = 0,
            .status = uhci_td_status(low_speed, false),
            .token = uhci_token(data_in ? USB_PID_IN : USB_PID_OUT,
                                address, 0, toggle, packet_length),
            .buffer = host.control_buffer_physical + 64 + offset,
        };
        td_count++;
        offset = (uint16_t)(offset + packet_length);
        toggle ^= 1;
    }

    uint8_t status_pid = data_in ? USB_PID_OUT : USB_PID_IN;
    host.td_pool[td_count] = (struct uhci_td) {
        .link = UHCI_LINK_TERMINATE,
        .status = uhci_td_status(low_speed, true),
        .token = uhci_token(status_pid, address, 0, 1, 0),
        .buffer = 0,
    };
    td_count++;

    for (uint32_t i = 0; i + 1 < td_count; i++) {
        host.td_pool[i].link = (host.td_pool_physical +
                                (i + 1) * sizeof(struct uhci_td)) |
                               UHCI_LINK_DEPTH;
    }
    __asm__ volatile("" : : : "memory");
    schedule_all_frames(host.td_pool_physical);

    bool success = uhci_wait_transfer(td_count);
    if (!success) {
        return false;
    }

    if (data_in && length != 0) {
        uint8_t *destination = data;
        for (uint16_t i = 0; i < length; i++) {
            destination[i] = transfer_data[i];
        }
    }
    return true;
}

static bool usb_get_descriptor(uint8_t address, uint8_t max_packet,
                               bool low_speed, uint8_t descriptor_type,
                               uint8_t descriptor_index, void *buffer,
                               uint16_t length)
{
    struct usb_setup_packet setup = {
        .request_type = 0x80,
        .request = USB_REQ_GET_DESCRIPTOR,
        .value = (uint16_t)((descriptor_type << 8) | descriptor_index),
        .index = 0,
        .length = length,
    };
    return uhci_control(address, max_packet, low_speed, &setup,
                        buffer, length);
}

static bool usb_set_address(uint8_t address, uint8_t max_packet,
                            bool low_speed, uint8_t new_address)
{
    struct usb_setup_packet setup = {
        .request_type = 0x00,
        .request = USB_REQ_SET_ADDRESS,
        .value = new_address,
        .index = 0,
        .length = 0,
    };
    return uhci_control(address, max_packet, low_speed, &setup, 0, 0);
}

static bool usb_set_configuration(uint8_t address, uint8_t max_packet,
                                  bool low_speed, uint8_t configuration)
{
    struct usb_setup_packet setup = {
        .request_type = 0x00,
        .request = USB_REQ_SET_CONFIG,
        .value = configuration,
        .index = 0,
        .length = 0,
    };
    return uhci_control(address, max_packet, low_speed, &setup, 0, 0);
}

static bool usb_set_boot_protocol(uint8_t address, uint8_t max_packet,
                                  bool low_speed, uint8_t interface_number)
{
    struct usb_setup_packet setup = {
        .request_type = 0x21,
        .request = USB_REQ_SET_PROTOCOL,
        .value = 0,
        .index = interface_number,
        .length = 0,
    };
    return uhci_control(address, max_packet, low_speed, &setup, 0, 0);
}

static bool enumerate_pointer(uint8_t port_number)
{
    uint8_t device_descriptor[18];
    uint8_t configuration_header[9];
    uint8_t configuration[USB_MAX_CONFIG_SIZE];
    uint8_t hid_report_descriptor[USB_MAX_CONFIG_SIZE];
    uint8_t ep0_max_packet = 8;
    uint8_t address = 0;

    if (!usb_get_descriptor(0, ep0_max_packet, host.low_speed,
                            USB_DESC_DEVICE, 0, device_descriptor, 8)) {
        return false;
    }
    if (device_descriptor[0] < 8 || device_descriptor[1] != USB_DESC_DEVICE ||
        device_descriptor[7] == 0 || device_descriptor[7] > 64 ||
        (host.low_speed && device_descriptor[7] > 8)) {
        return false;
    }
    ep0_max_packet = device_descriptor[7];
    if (!usb_get_descriptor(0, ep0_max_packet, host.low_speed,
                            USB_DESC_DEVICE, 0, device_descriptor,
                            sizeof(device_descriptor))) {
        return false;
    }

    address = 1;
    if (!usb_set_address(0, ep0_max_packet, host.low_speed, address)) {
        return false;
    }
    pit_delay_ms(2);

    if (!usb_get_descriptor(address, ep0_max_packet, host.low_speed,
                            USB_DESC_CONFIGURATION, 0,
                            configuration_header,
                            sizeof(configuration_header))) {
        return false;
    }
    uint16_t total_length = (uint16_t)configuration_header[2] |
                            ((uint16_t)configuration_header[3] << 8);
    if (total_length < sizeof(configuration_header) ||
        total_length > sizeof(configuration)) {
        return false;
    }
    if (!usb_get_descriptor(address, ep0_max_packet, host.low_speed,
                            USB_DESC_CONFIGURATION, 0,
                            configuration, total_length)) {
        return false;
    }

    uint8_t configuration_value = configuration[5];
    uint8_t interface_number = 0xFF;
    uint8_t endpoint_address = 0;
    uint16_t endpoint_packet = 0;
    uint8_t current_interface = 0xFF;
    uint8_t interface_subclass = 0;
    uint8_t interface_protocol = 0;
    uint8_t selected_subclass = 0;
    uint8_t selected_protocol = 0;
    uint16_t report_descriptor_length = 0;
    bool pointer_interface = false;
    bool boot_mouse_interface = false;
    bool selected_boot_mouse = false;

    for (uint16_t offset = 0; offset + 2 <= total_length;) {
        uint8_t descriptor_length = configuration[offset];
        uint8_t descriptor_type = configuration[offset + 1];
        if (descriptor_length < 2 || offset + descriptor_length > total_length) {
            return false;
        }

        if (descriptor_type == 4 && descriptor_length >= 9) {
            current_interface = configuration[offset + 2];
            interface_subclass = configuration[offset + 6];
            interface_protocol = configuration[offset + 7];
            boot_mouse_interface = configuration[offset + 3] == 0 &&
                                   configuration[offset + 5] == 3 &&
                                   interface_subclass == 1 &&
                                   interface_protocol == 2;
            pointer_interface = configuration[offset + 3] == 0 &&
                                configuration[offset + 5] == 3 &&
                                (boot_mouse_interface ||
                                 (interface_subclass == 0 &&
                                  interface_protocol == 0));
            if (pointer_interface && interface_number == 0xFF) {
                interface_number = current_interface;
                selected_subclass = interface_subclass;
                selected_protocol = interface_protocol;
                selected_boot_mouse = boot_mouse_interface;
            }
        } else if (descriptor_type == USB_DESC_HID && descriptor_length >= 9 &&
                   pointer_interface && current_interface == interface_number &&
                   configuration[offset + 6] == USB_DESC_HID_REPORT) {
            report_descriptor_length =
                (uint16_t)configuration[offset + 7] |
                ((uint16_t)configuration[offset + 8] << 8);
        } else if (descriptor_type == 5 && descriptor_length >= 7 &&
                   pointer_interface && current_interface == interface_number) {
            uint8_t address_value = configuration[offset + 2];
            uint8_t attributes = configuration[offset + 3];
            uint16_t packet = (uint16_t)configuration[offset + 4] |
                              ((uint16_t)configuration[offset + 5] << 8);
            packet &= 0x07FFu;
            if ((address_value & 0x80u) != 0 &&
                (attributes & 3u) == 3u && packet != 0 && packet <= 64 &&
                (!host.low_speed || packet <= 8)) {
                endpoint_address = address_value;
                endpoint_packet = packet;
                if (selected_boot_mouse) {
                    report_descriptor_length = 0;
                }
            }
        }
        offset = (uint16_t)(offset + descriptor_length);
    }

    if (endpoint_address == 0 || endpoint_packet == 0 || configuration_value == 0 ||
        (selected_boot_mouse && endpoint_packet < 3) ||
        (!selected_boot_mouse &&
         (report_descriptor_length == 0 ||
          report_descriptor_length > sizeof(hid_report_descriptor)))) {
        return false;
    }
    if (!usb_set_configuration(address, ep0_max_packet, host.low_speed,
                               configuration_value)) {
        return false;
    }

    if (selected_boot_mouse) {
        if (!usb_set_boot_protocol(address, ep0_max_packet, host.low_speed,
                                   interface_number) ||
            !usb_hid_pointer_init(true, 0, 0)) {
            return false;
        }
    } else {
        struct usb_setup_packet request = {
            .request_type = 0x81,
            .request = USB_REQ_GET_DESCRIPTOR,
            .value = (uint16_t)(USB_DESC_HID_REPORT << 8),
            .index = interface_number,
            .length = report_descriptor_length,
        };
        if (!uhci_control(address, ep0_max_packet, host.low_speed,
                          &request, hid_report_descriptor,
                          report_descriptor_length) ||
            !usb_hid_pointer_init(false, hid_report_descriptor,
                                  report_descriptor_length)) {
            return false;
        }
    }

    host.device = (struct usb_device_info) {
        .address = address,
        .vendor_id = (uint16_t)device_descriptor[8] |
                     ((uint16_t)device_descriptor[9] << 8),
        .product_id = (uint16_t)device_descriptor[10] |
                      ((uint16_t)device_descriptor[11] << 8),
        .device_class = 3,
        .device_subclass = selected_subclass,
        .device_protocol = selected_protocol,
        .interface_number = interface_number,
        .endpoint_in = endpoint_address,
        .endpoint_max_packet = endpoint_packet,
        .endpoint0_max_packet = ep0_max_packet,
        .low_speed = host.low_speed,
    };
    host.port_number = port_number;
    host.mouse_toggle = 0;
    host.mouse_td = (struct uhci_td *)host.mouse_buffer;
    host.mouse_td_physical = host.mouse_buffer_physical;
    host.pointer_ready = true;
    return true;
}

static void uhci_arm_mouse_transfer(void)
{
    struct uhci_td *td = host.mouse_td;
    *td = (struct uhci_td) {
        .link = UHCI_LINK_TERMINATE,
        .status = uhci_td_status(host.low_speed, true),
        .token = uhci_token(USB_PID_IN, host.device.address,
                            host.device.endpoint_in & 0x0Fu,
                            host.mouse_toggle,
                            host.device.endpoint_max_packet),
        .buffer = host.mouse_buffer_physical + 64,
    };
    host.mouse_buffer[64] = 0;
    host.mouse_buffer[65] = 0;
    host.mouse_buffer[66] = 0;
    __asm__ volatile("" : : : "memory");
    schedule_all_frames(host.mouse_td_physical);
}

bool uhci_init(void)
{
    host = (struct uhci_host){0};
    if (!uhci_controller_start()) {
        return false;
    }

    for (uint8_t port = 0; port < USB_ROOT_PORTS; port++) {
        if (!uhci_reset_port(port)) {
            continue;
        }
        if (enumerate_pointer(port)) {
            uhci_arm_mouse_transfer();
            return true;
        }
        /* Reset again to clear any address assigned to an unsupported device. */
        uhci_reset_port(port);
    }
    return true;
}

void uhci_poll(void)
{
    if (!host.initialized || !host.pointer_ready || host.mouse_td == 0) {
        return;
    }

    uint32_t status = host.mouse_td->status;
    if ((status & UHCI_TD_ACTIVE) != 0) {
        return; /* NAK keeps the transfer active and the controller retries. */
    }

    if ((status & UHCI_TD_ERRORS) == 0) {
        uint8_t *report = host.mouse_buffer + 64;
        uint16_t actual = uhci_actual_length(status);
        usb_hid_pointer_report(report, actual);
        host.mouse_toggle ^= 1u;
    }

    uhci_arm_mouse_transfer();
}

bool uhci_pointer_connected(void)
{
    return host.pointer_ready;
}

bool uhci_device_get(uint64_t index, struct usb_device_info *device)
{
    if (index != 0 || device == 0 || !host.pointer_ready) {
        return false;
    }
    *device = host.device;
    return true;
}

bool uhci_control_transfer(const struct usb_device_info *device,
                           const struct usb_setup_packet *setup, void *data)
{
    if (!host.initialized || device == 0 || setup == 0 ||
        device->endpoint0_max_packet == 0) {
        return false;
    }

    bool pointer_was_scheduled = host.pointer_ready;
    if (pointer_was_scheduled) {
        clear_frame_list();
    }

    bool result = uhci_control(device->address,
                               device->endpoint0_max_packet,
                               device->low_speed, setup, data, setup->length);
    if (pointer_was_scheduled) {
        uhci_arm_mouse_transfer();
    }
    return result;
}

bool uhci_interrupt_in(const struct usb_device_info *device,
                       uint8_t endpoint_address, uint8_t *data_toggle,
                       void *buffer, uint16_t capacity,
                       uint16_t *actual_length)
{
    if (!host.initialized || device == 0 || data_toggle == 0 ||
        buffer == 0 || capacity == 0 || actual_length == 0 ||
        (endpoint_address & 0x80u) == 0 ||
        (endpoint_address & 0x0Fu) == 0 ||
        device->endpoint_max_packet == 0) {
        return false;
    }

    uint16_t packet_length = capacity;
    if (packet_length > device->endpoint_max_packet) {
        packet_length = device->endpoint_max_packet;
    }
    if (packet_length > 64 || packet_length > USB_CONTROL_BUFFER_SIZE - 64) {
        return false;
    }

    bool pointer_was_scheduled = host.pointer_ready;
    if (pointer_was_scheduled) {
        clear_frame_list();
    }

    host.td_pool[0] = (struct uhci_td) {
        .link = UHCI_LINK_TERMINATE,
        .status = uhci_td_status(device->low_speed, true),
        .token = uhci_token(USB_PID_IN, device->address,
                            endpoint_address & 0x0Fu,
                            *data_toggle, packet_length),
        .buffer = host.control_buffer_physical + 64,
    };
    __asm__ volatile("" : : : "memory");
    schedule_all_frames(host.td_pool_physical);

    bool success = uhci_wait_transfer(1);
    if (success) {
        uint32_t status = host.td_pool[0].status;
        uint16_t actual = uhci_actual_length(status);
        if (actual > packet_length) {
            actual = packet_length;
        }
        uint8_t *destination = buffer;
        for (uint16_t i = 0; i < actual; i++) {
            destination[i] = host.control_buffer[64 + i];
        }
        *actual_length = actual;
        *data_toggle ^= 1u;
    }

    if (pointer_was_scheduled) {
        uhci_arm_mouse_transfer();
    }
    return success;
}
