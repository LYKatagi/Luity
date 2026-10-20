#include <drivers/input/mouse/mouse.h>
#include <drivers/usb/hid_pointer.h>
#include <events/event.h>

#define HID_USAGE_DESKTOP 0x01u
#define HID_USAGE_BUTTON 0x09u
#define HID_USAGE_DIGITIZER 0x0Du
#define HID_USAGE_X 0x30u
#define HID_USAGE_Y 0x31u
#define HID_USAGE_TIP_SWITCH 0x42u
#define HID_USAGE_CONTACT_COUNT 0x54u
#define HID_USAGE_TOUCHPAD 0x05u

#define HID_MAX_LOCAL_USAGES 32u
#define HID_GLOBAL_STACK 8u

struct hid_global_state {
    uint16_t usage_page;
    int32_t logical_minimum;
    int32_t logical_maximum;
    uint32_t report_size;
    uint32_t report_count;
    uint8_t report_id;
};

struct hid_field {
    uint16_t bit_offset;
    uint8_t bit_size;
    uint8_t report_id;
    int32_t logical_minimum;
    int32_t logical_maximum;
    bool present;
};

struct hid_pointer_state {
    enum usb_hid_pointer_kind kind;
    struct hid_field x;
    struct hid_field y;
    struct hid_field button;
    struct hid_field contact;
    bool contact_is_count;
    bool touch_active;
    int64_t touch_start_x;
    int64_t touch_start_y;
    int64_t last_x;
    int64_t last_y;
};

static struct hid_pointer_state pointer;

static void submit_relative(int16_t dx, int16_t dy, uint8_t buttons)
{
    uint64_t flags;
    __asm__ volatile("pushfq; popq %0; cli" : "=r"(flags) : : "memory");
    mouse_handle_relative(dx, dy, buttons);
    if ((flags & (1ull << 9)) != 0) {
        __asm__ volatile("sti" : : : "memory");
    }
}

static void submit_absolute(int64_t x, int64_t y, uint8_t buttons)
{
    uint64_t flags;
    __asm__ volatile("pushfq; popq %0; cli" : "=r"(flags) : : "memory");
    mouse_handle_absolute(x, y, buttons);
    if ((flags & (1ull << 9)) != 0) {
        __asm__ volatile("sti" : : : "memory");
    }
}

static int32_t sign_extend(uint32_t value, uint32_t size)
{
    if (size == 0 || size >= 32) {
        return (int32_t)value;
    }
    uint32_t sign = 1u << (size - 1u);
    return (int32_t)((value ^ sign) - sign);
}

static uint32_t item_unsigned(const uint8_t *data, uint8_t size)
{
    uint32_t value = 0;
    for (uint8_t i = 0; i < size; i++) {
        value |= (uint32_t)data[i] << (i * 8u);
    }
    return value;
}

static int32_t item_signed(const uint8_t *data, uint8_t size)
{
    if (size == 0) {
        return 0;
    }
    return sign_extend(item_unsigned(data, size), size * 8u);
}

static uint32_t local_usage(uint32_t raw, uint8_t size,
                            uint16_t usage_page)
{
    if (size == 4 && (raw >> 16) != 0) {
        return raw;
    }
    return ((uint32_t)usage_page << 16) | (raw & 0xFFFFu);
}

static uint32_t usage_at(const uint32_t *usages, uint8_t usage_count,
                         bool has_minimum, uint32_t usage_minimum,
                         bool has_maximum, uint32_t usage_maximum,
                         uint32_t index)
{
    if (index < usage_count) {
        return usages[index];
    }
    if (usage_count == 1) {
        return usages[0];
    }
    if (has_minimum && has_maximum && usage_minimum <= usage_maximum &&
        usage_minimum + index <= usage_maximum) {
        return usage_minimum + index;
    }
    return 0;
}

static void field_set(struct hid_field *field,
                      const struct hid_global_state *global,
                      uint32_t offset)
{
    if (field->present || global->report_size == 0 ||
        global->report_size > 32 || offset > UINT16_MAX) {
        return;
    }
    field->bit_offset = (uint16_t)offset;
    field->bit_size = (uint8_t)global->report_size;
    field->report_id = global->report_id;
    field->logical_minimum = global->logical_minimum;
    field->logical_maximum = global->logical_maximum;
    field->present = true;
}

static bool hid_parse_report_descriptor(const uint8_t *descriptor,
                                        uint16_t length)
{
    struct hid_field relative_x = {0};
    struct hid_field relative_y = {0};
    struct hid_field touch_x = {0};
    struct hid_field touch_y = {0};
    struct hid_global_state global = {0};
    struct hid_global_state stack[HID_GLOBAL_STACK];
    uint8_t stack_depth = 0;
    uint32_t report_offsets[256] = {0};
    uint32_t usages[HID_MAX_LOCAL_USAGES];
    uint8_t usage_count = 0;
    bool has_usage_minimum = false;
    bool has_usage_maximum = false;
    uint32_t usage_minimum = 0;
    uint32_t usage_maximum = 0;
    bool application_touchpad = false;

    for (uint16_t offset = 0; offset < length;) {
        uint8_t prefix = descriptor[offset++];
        if (prefix == 0xFE) {
            if (offset + 2 > length) {
                return false;
            }
            uint8_t data_length = descriptor[offset++];
            offset++; /* long-item tag */
            if ((uint32_t)offset + data_length > length) {
                return false;
            }
            offset = (uint16_t)(offset + data_length);
            continue;
        }

        uint8_t data_size = prefix & 3u;
        if (data_size == 3) {
            data_size = 4;
        }
        uint8_t item_type = (prefix >> 2) & 3u;
        uint8_t tag = prefix >> 4;
        if ((uint32_t)offset + data_size > length) {
            return false;
        }
        const uint8_t *data = descriptor + offset;
        uint32_t value = item_unsigned(data, data_size);

        if (item_type == 1) { /* Global item */
            switch (tag) {
            case 0: global.usage_page = (uint16_t)value; break;
            case 1: global.logical_minimum = item_signed(data, data_size); break;
            case 2:
                global.logical_maximum = global.logical_minimum < 0
                    ? item_signed(data, data_size)
                    : (int32_t)value;
                break;
            case 7: global.report_size = value; break;
            case 8: global.report_id = (uint8_t)value; break;
            case 9: global.report_count = value; break;
            case 10:
                if (stack_depth >= HID_GLOBAL_STACK) {
                    return false;
                }
                stack[stack_depth++] = global;
                break;
            case 11:
                if (stack_depth == 0) {
                    return false;
                }
                global = stack[--stack_depth];
                break;
            default: break;
            }
        } else if (item_type == 2) { /* Local item */
            if (tag == 0 && usage_count < HID_MAX_LOCAL_USAGES) {
                usages[usage_count++] = local_usage(
                    value, data_size, global.usage_page);
            } else if (tag == 1) {
                has_usage_minimum = true;
                usage_minimum = local_usage(value, data_size,
                                            global.usage_page);
            } else if (tag == 2) {
                has_usage_maximum = true;
                usage_maximum = local_usage(value, data_size,
                                            global.usage_page);
            }
        } else if (item_type == 0) { /* Main item */
            if (tag == 10 && (value & 0xFFu) == 1u && usage_count != 0) {
                uint32_t app_usage = usages[0];
                application_touchpad |=
                    (app_usage >> 16) == HID_USAGE_DIGITIZER &&
                    (app_usage & 0xFFFFu) == HID_USAGE_TOUCHPAD;
            } else if (tag == 8) { /* Input */
                if (global.report_size > 32 || global.report_count > 256 ||
                    global.report_size * global.report_count > 8192) {
                    return false;
                }
                bool constant = (value & 1u) != 0;
                bool variable = (value & 2u) != 0;
                bool relative = (value & 4u) != 0;
                uint32_t start = report_offsets[global.report_id];
                if (!constant && variable) {
                    for (uint32_t field_index = 0;
                         field_index < global.report_count; field_index++) {
                        uint32_t usage = usage_at(
                            usages, usage_count, has_usage_minimum,
                            usage_minimum, has_usage_maximum,
                            usage_maximum, field_index);
                        uint16_t page = (uint16_t)(usage >> 16);
                        uint16_t id = (uint16_t)usage;
                        if (page == HID_USAGE_DESKTOP && id == HID_USAGE_X &&
                            relative) {
                            field_set(&relative_x, &global,
                                      start + field_index * global.report_size);
                        } else if (page == HID_USAGE_DESKTOP &&
                                   id == HID_USAGE_Y &&
                                   relative) {
                            field_set(&relative_y, &global,
                                      start + field_index * global.report_size);
                        } else if (page == HID_USAGE_DESKTOP &&
                                   id == HID_USAGE_X &&
                                   !relative && application_touchpad) {
                            field_set(&touch_x, &global,
                                      start + field_index * global.report_size);
                        } else if (page == HID_USAGE_DESKTOP &&
                                   id == HID_USAGE_Y &&
                                   !relative && application_touchpad) {
                            field_set(&touch_y, &global,
                                      start + field_index * global.report_size);
                        } else if (page == HID_USAGE_BUTTON && id == 1 &&
                                   pointer.button.present == false) {
                            field_set(&pointer.button, &global,
                                      start + field_index * global.report_size);
                        } else if (page == HID_USAGE_DIGITIZER &&
                                   (id == HID_USAGE_TIP_SWITCH ||
                                    id == HID_USAGE_CONTACT_COUNT) &&
                                   pointer.contact.present == false) {
                            field_set(&pointer.contact, &global,
                                      start + field_index * global.report_size);
                            pointer.contact_is_count =
                                id == HID_USAGE_CONTACT_COUNT;
                        }
                    }
                }
                report_offsets[global.report_id] +=
                    global.report_size * global.report_count;
            } else if (tag == 10 && usage_count != 0) {
                /* Non-application collections do not change input fields. */
            }

            usage_count = 0;
            has_usage_minimum = false;
            has_usage_maximum = false;
        }

        offset = (uint16_t)(offset + data_size);
    }

    if (application_touchpad && touch_x.present && touch_y.present &&
        pointer.contact.present &&
        touch_x.report_id == touch_y.report_id &&
        touch_x.report_id == pointer.contact.report_id &&
        touch_x.logical_maximum > touch_x.logical_minimum &&
        touch_y.logical_maximum > touch_y.logical_minimum) {
        pointer.x = touch_x;
        pointer.y = touch_y;
        pointer.kind = USB_HID_POINTER_TOUCHPAD;
        return true;
    }
    if (relative_x.present && relative_y.present) {
        pointer.x = relative_x;
        pointer.y = relative_y;
    }
    if (pointer.x.report_id == pointer.y.report_id &&
        pointer.x.present && pointer.y.present) {
        pointer.kind = USB_HID_POINTER_RELATIVE_MOUSE;
        return true;
    }
    return false;
}

bool usb_hid_pointer_init(bool boot_mouse, const uint8_t *descriptor,
                          uint16_t descriptor_length)
{
    pointer = (struct hid_pointer_state){0};
    if (boot_mouse) {
        pointer.kind = USB_HID_POINTER_BOOT_MOUSE;
        return true;
    }
    if (descriptor == 0 || descriptor_length == 0) {
        return false;
    }
    return hid_parse_report_descriptor(descriptor, descriptor_length);
}

enum usb_hid_pointer_kind usb_hid_pointer_kind(void)
{
    return pointer.kind;
}

static bool extract_field(const struct hid_field *field,
                          const uint8_t *report, uint16_t length,
                          uint8_t report_id, int32_t *value)
{
    if (!field->present || field->report_id != report_id ||
        field->bit_size == 0 || field->bit_size > 32) {
        return false;
    }
    uint32_t bit = field->bit_offset;
    uint32_t last_bit = bit + field->bit_size;
    uint32_t bytes_needed = (last_bit + 7u) / 8u;
    if (bytes_needed > length) {
        return false;
    }
    uint32_t raw = 0;
    for (uint32_t i = 0; i < field->bit_size; i++) {
        uint32_t source_bit = bit + i;
        if ((report[source_bit / 8u] & (1u << (source_bit % 8u))) != 0) {
            raw |= 1u << i;
        }
    }
    *value = field->logical_minimum < 0
        ? sign_extend(raw, field->bit_size)
        : (int32_t)raw;
    return true;
}

static int64_t scale_axis(int32_t value, const struct hid_field *field,
                          int64_t screen_maximum)
{
    if (value < field->logical_minimum) {
        value = field->logical_minimum;
    } else if (value > field->logical_maximum) {
        value = field->logical_maximum;
    }
    int64_t source_range =
        (int64_t)field->logical_maximum - field->logical_minimum;
    if (source_range <= 0 || screen_maximum <= 0) {
        return 0;
    }
    return ((int64_t)value - field->logical_minimum) * screen_maximum /
           source_range;
}

static void submit_mouse_report(const uint8_t *report, uint16_t length,
                                uint8_t report_id)
{
    int32_t x = 0;
    int32_t y = 0;
    int32_t button = 0;
    (void)extract_field(&pointer.x, report, length, report_id, &x);
    (void)extract_field(&pointer.y, report, length, report_id, &y);
    (void)extract_field(&pointer.button, report, length, report_id, &button);
    submit_relative((int16_t)x, (int16_t)y,
                    button != 0 ? MOUSE_BUTTON_LEFT : 0);
}

bool usb_hid_pointer_report(const uint8_t *report, uint16_t length)
{
    if (report == 0) {
        return false;
    }
    if (pointer.kind == USB_HID_POINTER_BOOT_MOUSE) {
        if (length < 3) {
            return false;
        }
        submit_relative((int8_t)report[1], (int8_t)report[2],
                        report[0] & 0x07u);
        return true;
    }

    if (pointer.kind != USB_HID_POINTER_RELATIVE_MOUSE &&
        pointer.kind != USB_HID_POINTER_TOUCHPAD) {
        return false;
    }

    uint8_t report_id = 0;
    if (pointer.x.report_id != 0) {
        if (length < 2 || report[0] != pointer.x.report_id) {
            return false;
        }
        report_id = report[0];
        report++;
        length--;
    }

    if (pointer.kind == USB_HID_POINTER_RELATIVE_MOUSE) {
        submit_mouse_report(report, length, report_id);
        return true;
    }

    int32_t raw_x = 0;
    int32_t raw_y = 0;
    int32_t touch_value = 0;
    int32_t button_value = 0;
    if (!extract_field(&pointer.x, report, length, report_id, &raw_x) ||
        !extract_field(&pointer.y, report, length, report_id, &raw_y) ||
        !extract_field(&pointer.contact, report, length, report_id,
                       &touch_value)) {
        return false;
    }
    (void)extract_field(&pointer.button, report, length, report_id,
                        &button_value);

    int64_t screen_max_x;
    int64_t screen_max_y;
    mouse_get_bounds(&screen_max_x, &screen_max_y);
    int64_t x = scale_axis(raw_x, &pointer.x, screen_max_x);
    int64_t y = scale_axis(raw_y, &pointer.y, screen_max_y);
    bool touching = touch_value != 0;
    uint8_t buttons = button_value != 0 ? MOUSE_BUTTON_LEFT : 0;

    if (touching) {
        if (!pointer.touch_active) {
            pointer.touch_start_x = x;
            pointer.touch_start_y = y;
        }
        pointer.touch_active = true;
        pointer.last_x = x;
        pointer.last_y = y;
        submit_absolute(x, y, buttons);
    } else {
        bool was_touch_active = pointer.touch_active;
        if (was_touch_active && buttons == 0) {
            int64_t dx = pointer.last_x - pointer.touch_start_x;
            int64_t dy = pointer.last_y - pointer.touch_start_y;
            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;
            if (dx <= 12 && dy <= 12) {
                submit_absolute(pointer.last_x, pointer.last_y,
                                MOUSE_BUTTON_LEFT);
                submit_absolute(pointer.last_x, pointer.last_y, 0);
            }
        }
        pointer.touch_active = false;
        if (buttons != 0) {
            submit_absolute(pointer.last_x, pointer.last_y, buttons);
        } else if (was_touch_active || mouse_get_state()->buttons != 0) {
            /* Release any previously held physical button. */
            submit_absolute(pointer.last_x, pointer.last_y, 0);
        }
    }
    return true;
}
