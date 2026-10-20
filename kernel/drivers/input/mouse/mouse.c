#include <drivers/input/mouse/mouse.h>
#include <drivers/input/ps2/controller.h>
#include <events/event.h>

#define MOUSE_ACK 0xFA
#define MOUSE_CMD_SET_DEFAULTS 0xF6
#define MOUSE_CMD_ENABLE_DATA 0xF4
#define MOUSE_CMD_SET_RATE 0xF3
#define MOUSE_CMD_GET_INFO 0xE9
#define MOUSE_CMD_SET_RESOLUTION 0xE8
#define MOUSE_CMD_SET_SCALE_1_1 0xE6

#define SYNAPTICS_ABSOLUTE_MODE 0x80
#define SYNAPTICS_MODE_RATE 0x14
#define SYNAPTICS_QUERY_MODEL 0x03
#define SYNAPTICS_AXIS_MAX 6143

static struct mouse_state state = {
    .x = 0,
    .y = 0,
    .delta_x = 0,
    .delta_y = 0,
    .buttons = 0,
    .left = false,
    .right = false,
    .middle = false};

static uint8_t packet[6];
static uint8_t packet_index = 0;
static bool synaptics_absolute = false;
static bool synaptics_new_absolute = false;
static bool synaptics_touch_active = false;
static int64_t synaptics_touch_start_x;
static int64_t synaptics_touch_start_y;
static int64_t synaptics_last_x;
static int64_t synaptics_last_y;
static int64_t max_x = INT64_MAX;
static int64_t max_y = INT64_MAX;

static bool mouse_send_command(uint8_t command)
{
    for (unsigned int attempt = 0; attempt < 3; attempt++)
    {
        if (!ps2_write_mouse(command))
        {
            return false;
        }

        bool resend = false;
        for (uint32_t timeout = 0; timeout < PS2_POLL_LIMIT; timeout++)
        {
            uint8_t status = ps2_read_status();
            if ((status & PS2_STATUS_OUTPUT_FULL) == 0)
            {
                continue;
            }

            uint8_t response = ps2_read_data();
            if ((status & PS2_STATUS_AUXILIARY) == 0 ||
                (status & (PS2_STATUS_TIMEOUT | PS2_STATUS_PARITY)) != 0)
            {
                continue;
            }

            if (response == MOUSE_ACK)
            {
                return true;
            }
            if (response == 0xFE)
            {
                resend = true;
                break;
            }

            return false;
        }

        if (!resend)
        {
            return false;
        }
    }

    return false;
}

static bool mouse_send_parameter(uint8_t command, uint8_t parameter)
{
    if (!mouse_send_command(command)) {
        return false;
    }
    return mouse_send_command(parameter);
}

static bool mouse_read_auxiliary_byte(uint8_t *value)
{
    if (value == 0) {
        return false;
    }

    for (uint32_t timeout = 0; timeout < PS2_POLL_LIMIT; timeout++) {
        uint8_t status = ps2_read_status();
        if ((status & PS2_STATUS_OUTPUT_FULL) == 0) {
            continue;
        }

        uint8_t response = ps2_read_data();
        if ((status & (PS2_STATUS_TIMEOUT | PS2_STATUS_PARITY)) != 0 ||
            (status & PS2_STATUS_AUXILIARY) == 0) {
            continue;
        }

        *value = response;
        return true;
    }
    return false;
}

static bool synaptics_sliced_command(uint8_t command)
{
    if (!mouse_send_command(MOUSE_CMD_SET_SCALE_1_1)) {
        return false;
    }

    for (int shift = 6; shift >= 0; shift -= 2) {
        uint8_t part = (uint8_t)((command >> shift) & 3u);
        if (!mouse_send_parameter(MOUSE_CMD_SET_RESOLUTION, part)) {
            return false;
        }
    }
    return true;
}

static bool synaptics_set_mode(uint8_t mode)
{
    return synaptics_sliced_command(mode) &&
           mouse_send_parameter(MOUSE_CMD_SET_RATE, SYNAPTICS_MODE_RATE);
}

static bool synaptics_query(uint8_t query, uint8_t response[3])
{
    return synaptics_sliced_command(query) &&
           mouse_send_command(MOUSE_CMD_GET_INFO) &&
           mouse_read_auxiliary_byte(&response[0]) &&
           mouse_read_auxiliary_byte(&response[1]) &&
           mouse_read_auxiliary_byte(&response[2]);
}

static bool synaptics_probe(void)
{
    uint8_t response[3];
    uint8_t model[3];
    for (uint8_t i = 0; i < 4; i++) {
        if (!mouse_send_parameter(MOUSE_CMD_SET_RESOLUTION, 0)) {
            return false;
        }
    }

    if (!mouse_send_command(MOUSE_CMD_GET_INFO)) {
        return false;
    }
    for (uint8_t i = 0; i < sizeof(response); i++) {
        if (!mouse_read_auxiliary_byte(&response[i])) {
            return false;
        }
    }

    /* Synaptics identifies itself with 0x47 in the middle GETINFO byte. */
    if (response[1] != 0x47) {
        return false;
    }
    if (!synaptics_query(SYNAPTICS_QUERY_MODEL, model)) {
        return false;
    }
    /* Model bit 7 selects the newer absolute packet coordinate layout. */
    synaptics_new_absolute = (model[2] & 0x80u) != 0;
    return synaptics_set_mode(SYNAPTICS_ABSOLUTE_MODE);
}

bool mouse_init(void)
{
    synaptics_absolute = false;
    synaptics_new_absolute = false;
    synaptics_touch_active = false;

    /* Start from the standard PS/2 defaults before probing extensions. */
    if (!mouse_send_command(MOUSE_CMD_SET_DEFAULTS))
    {
        return false;
    }

    if (synaptics_probe()) {
        synaptics_absolute = true;
        if (!mouse_send_command(MOUSE_CMD_ENABLE_DATA)) {
            synaptics_absolute = false;
        }
    }

    if (!synaptics_absolute) {
        /* A failed probe may have changed the resolution; restore defaults. */
        if (!mouse_send_command(MOUSE_CMD_SET_DEFAULTS) ||
            !mouse_send_command(MOUSE_CMD_ENABLE_DATA)) {
            return false;
        }
    }

    state.x = 0;
    state.y = 0;
    state.delta_x = 0;
    state.delta_y = 0;
    state.buttons = 0;
    state.left = false;
    state.right = false;
    state.middle = false;

    packet_index = 0;

    return true;
}

void mouse_set_bounds(uint64_t width, uint64_t height)
{
    if (width == 0 || height == 0)
    {
        max_x = 0;
        max_y = 0;
        state.x = 0;
        state.y = 0;
        return;
    }

    max_x = width - 1 > INT64_MAX ? INT64_MAX : (int64_t)(width - 1);
    max_y = height - 1 > INT64_MAX ? INT64_MAX : (int64_t)(height - 1);
    state.x = max_x / 2;
    state.y = max_y / 2;
}

static int64_t add_clamped(int64_t position, int32_t delta, int64_t maximum)
{
    if (delta > 0 && position > maximum - delta)
    {
        return maximum;
    }
    if (delta < 0 && position < -(int64_t)delta)
    {
        return 0;
    }

    position += delta;
    if (position < 0)
    {
        return 0;
    }
    if (position > maximum)
    {
        return maximum;
    }
    return position;
}

void mouse_handle_byte(uint8_t byte)
{
    if (synaptics_absolute) {
        if (packet_index == 0) {
            /* Synaptics absolute packets carry 0x80 in byte 0 and 0xC0 in
             * byte 3; the two signatures also let us recover packet framing.
             */
            if ((byte & 0xC8u) != 0x80u) {
                return;
            }
        }

        packet[packet_index++] = byte;
        if (packet_index < 6) {
            return;
        }
        packet_index = 0;

        if ((packet[3] & 0xC8u) != 0xC0u) {
            return;
        }

        int32_t raw_x;
        int32_t raw_y;
        int32_t pressure;
        if (synaptics_new_absolute) {
            raw_x = ((int32_t)(packet[3] & 0x10u) << 8) |
                    ((int32_t)(packet[1] & 0x0Fu) << 8) | packet[4];
            raw_y = ((int32_t)(packet[3] & 0x20u) << 7) |
                    ((int32_t)(packet[1] & 0xF0u) << 4) | packet[5];
            pressure = packet[2];
        } else {
            raw_x = ((int32_t)(packet[1] & 0x1Fu) << 8) | packet[2];
            raw_y = ((int32_t)(packet[4] & 0x1Fu) << 8) | packet[5];
            pressure = ((packet[0] & 0x30u) << 2) | (packet[3] & 0x3Fu);
        }
        int64_t maximum_x;
        int64_t maximum_y;
        mouse_get_bounds(&maximum_x, &maximum_y);
        int64_t x = (int64_t)raw_x * maximum_x / SYNAPTICS_AXIS_MAX;
        int64_t y = (int64_t)(SYNAPTICS_AXIS_MAX - raw_y) * maximum_y /
                    SYNAPTICS_AXIS_MAX;
        uint8_t buttons = packet[0] & 0x03u;

        if (pressure != 0) {
            if (!synaptics_touch_active) {
                synaptics_touch_start_x = x;
                synaptics_touch_start_y = y;
            }
            synaptics_touch_active = true;
            synaptics_last_x = x;
            synaptics_last_y = y;
            mouse_handle_absolute(x, y, buttons);
        } else {
            bool was_touch_active = synaptics_touch_active;
            if (was_touch_active && buttons == 0) {
                int64_t dx = synaptics_last_x - synaptics_touch_start_x;
                int64_t dy = synaptics_last_y - synaptics_touch_start_y;
                if (dx < 0) dx = -dx;
                if (dy < 0) dy = -dy;
                if (dx <= 12 && dy <= 12) {
                    mouse_handle_absolute(synaptics_last_x,
                                          synaptics_last_y,
                                          MOUSE_BUTTON_LEFT);
                }
            }
            synaptics_touch_active = false;
            if (buttons != 0) {
                mouse_handle_absolute(synaptics_last_x, synaptics_last_y,
                                      buttons);
            } else if (was_touch_active || state.buttons != 0) {
                mouse_handle_absolute(synaptics_last_x, synaptics_last_y, 0);
            }
        }
        return;
    }

    /*
     * The first byte of a PS/2 packet always has bit 3 set.
     *
     * This also helps recover synchronization if a byte is lost.
     */
    if (packet_index == 0)
    {
        if ((byte & 0x08) == 0)
        {
            return;
        }
    }

    packet[packet_index++] = byte;

    if (packet_index < 3)
    {
        return;
    }

    packet_index = 0;

    uint8_t flags = packet[0];

    /*
     * Ignore packets with overflow.
     */
    if ((flags & 0x40) != 0 || (flags & 0x80) != 0)
    {
        return;
    }

    int16_t dx = (int16_t)packet[1];
    int16_t dy = (int16_t)packet[2];

    /*
     * Bit 4 = sinal de X
     * Bit 5 = sinal de Y
     */
    if ((flags & 0x10) != 0)
    {
        dx |= 0xFF00;
    }

    if ((flags & 0x20) != 0)
    {
        dy |= 0xFF00;
    }

    /* PS/2 Y grows upward; framebuffer coordinates grow downward. */
    mouse_handle_relative(dx, (int16_t)-dy, flags & 0x07);
}

static void mouse_publish_changes(int64_t old_x, int64_t old_y,
                                  uint8_t old_buttons)
{
    if (state.x != old_x || state.y != old_y) {
        struct kernel_event event = {
            .type = KERNEL_EVENT_MOUSE_MOVE,
            .x = state.x,
            .y = state.y,
            .button = 0,
            .buttons = state.buttons,
        };
        event_post(&event);
    }

    static const uint8_t button_masks[] = {
        MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE};
    for (uint64_t index = 0;
         index < sizeof(button_masks) / sizeof(button_masks[0]); index++) {
        uint8_t button = button_masks[index];
        bool was_pressed = (old_buttons & button) != 0;
        bool is_pressed = (state.buttons & button) != 0;
        if (was_pressed == is_pressed) {
            continue;
        }

        struct kernel_event event = {
            .type = is_pressed ? KERNEL_EVENT_MOUSE_BUTTON_DOWN
                               : KERNEL_EVENT_MOUSE_BUTTON_UP,
            .x = state.x,
            .y = state.y,
            .button = button,
            .buttons = state.buttons,
        };
        event_post(&event);
    }
}

void mouse_handle_relative(int16_t dx, int16_t dy, uint8_t buttons)
{
    int64_t old_x = state.x;
    int64_t old_y = state.y;
    uint8_t old_buttons = state.buttons;

    state.delta_x = dx;
    state.delta_y = dy;
    state.x = add_clamped(state.x, dx, max_x);
    state.y = add_clamped(state.y, dy, max_y);
    state.buttons = buttons & 0x07;
    state.left = (state.buttons & MOUSE_BUTTON_LEFT) != 0;
    state.right = (state.buttons & MOUSE_BUTTON_RIGHT) != 0;
    state.middle = (state.buttons & MOUSE_BUTTON_MIDDLE) != 0;
    mouse_publish_changes(old_x, old_y, old_buttons);
}

void mouse_handle_absolute(int64_t x, int64_t y, uint8_t buttons)
{
    int64_t old_x = state.x;
    int64_t old_y = state.y;
    uint8_t old_buttons = state.buttons;

    if (x < 0) {
        x = 0;
    } else if (x > max_x) {
        x = max_x;
    }
    if (y < 0) {
        y = 0;
    } else if (y > max_y) {
        y = max_y;
    }

    state.delta_x = (int16_t)(x - old_x);
    state.delta_y = (int16_t)(y - old_y);
    state.x = x;
    state.y = y;
    state.buttons = buttons & 0x07;
    state.left = (state.buttons & MOUSE_BUTTON_LEFT) != 0;
    state.right = (state.buttons & MOUSE_BUTTON_RIGHT) != 0;
    state.middle = (state.buttons & MOUSE_BUTTON_MIDDLE) != 0;
    mouse_publish_changes(old_x, old_y, old_buttons);
}

void mouse_get_bounds(int64_t *max_x_out, int64_t *max_y_out)
{
    if (max_x_out != 0) {
        *max_x_out = max_x;
    }
    if (max_y_out != 0) {
        *max_y_out = max_y;
    }
}

const struct mouse_state *mouse_get_state(void)
{
    return &state;
}
