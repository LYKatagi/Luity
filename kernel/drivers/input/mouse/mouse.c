#include <drivers/input/mouse/mouse.h>
#include <drivers/input/ps2/controller.h>
#include <events/event.h>

#define MOUSE_ACK       0xFA
#define MOUSE_CMD_SET_DEFAULTS 0xF6
#define MOUSE_CMD_ENABLE_DATA  0xF4

static struct mouse_state state = {
    .x = 0,
    .y = 0,
    .delta_x = 0,
    .delta_y = 0,
    .buttons = 0,
    .left = false,
    .right = false,
    .middle = false
};

static uint8_t packet[3];
static uint8_t packet_index = 0;
static int64_t max_x = INT64_MAX;
static int64_t max_y = INT64_MAX;

static bool mouse_send_command(uint8_t command)
{
    for (unsigned int attempt = 0; attempt < 3; attempt++) {
        if (!ps2_write_mouse(command)) {
            return false;
        }

        bool resend = false;
        for (uint32_t timeout = 0; timeout < PS2_POLL_LIMIT; timeout++) {
            uint8_t status = ps2_read_status();
            if ((status & PS2_STATUS_OUTPUT_FULL) == 0) {
                continue;
            }

            uint8_t response = ps2_read_data();
            if ((status & PS2_STATUS_AUXILIARY) == 0 ||
                (status & (PS2_STATUS_TIMEOUT | PS2_STATUS_PARITY)) != 0) {
                continue;
            }

            if (response == MOUSE_ACK) {
                return true;
            }
            if (response == 0xFE) {
                resend = true;
                break;
            }

            return false;
        }

        if (!resend) {
            return false;
        }
    }

    return false;
}

bool mouse_init(void)
{
    /*
     * Restaurar configuração padrão.
     */
    if (!mouse_send_command(MOUSE_CMD_SET_DEFAULTS)) {
        return false;
    }

    /*
     * Habilitar envio de dados.
     */
    if (!mouse_send_command(MOUSE_CMD_ENABLE_DATA)) {
        return false;
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
    if (width == 0 || height == 0) {
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
    if (delta > 0 && position > maximum - delta) {
        return maximum;
    }
    if (delta < 0 && position < -(int64_t)delta) {
        return 0;
    }

    position += delta;
    if (position < 0) {
        return 0;
    }
    if (position > maximum) {
        return maximum;
    }
    return position;
}

void mouse_handle_byte(uint8_t byte)
{
    /*
     * Primeiro byte de um pacote PS/2 possui o bit 3 sempre em 1.
     *
     * Isso também ajuda a recuperar a sincronização caso algum byte
     * seja perdido.
     */
    if (packet_index == 0) {
        if ((byte & 0x08) == 0) {
            return;
        }
    }

    packet[packet_index++] = byte;

    if (packet_index < 3) {
        return;
    }

    packet_index = 0;

    uint8_t flags = packet[0];

    /*
     * Ignorar pacotes com overflow.
     */
    if ((flags & 0x40) != 0 || (flags & 0x80) != 0) {
        return;
    }

    int64_t old_x = state.x;
    int64_t old_y = state.y;
    uint8_t old_buttons = state.buttons;

    int16_t dx = (int16_t)packet[1];
    int16_t dy = (int16_t)packet[2];

    /*
     * Bit 4 = sinal de X
     * Bit 5 = sinal de Y
     */
    if ((flags & 0x10) != 0) {
        dx |= 0xFF00;
    }

    if ((flags & 0x20) != 0) {
        dy |= 0xFF00;
    }

    state.delta_x = dx;
    state.delta_y = dy;

    /*
     * PS/2 considera Y positivo como movimento para cima.
     * Na tela do framebuffer, Y normalmente cresce para baixo.
     */
    state.x = add_clamped(state.x, dx, max_x);
    state.y = add_clamped(state.y, -(int32_t)dy, max_y);

    /*
     * Botões:
     *
     * bit 0 = esquerdo
     * bit 1 = direito
     * bit 2 = meio
     */
    state.buttons = flags & 0x07;

    state.left = (state.buttons & 0x01) != 0;
    state.right = (state.buttons & 0x02) != 0;
    state.middle = (state.buttons & 0x04) != 0;

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
        MOUSE_BUTTON_LEFT,
        MOUSE_BUTTON_RIGHT,
        MOUSE_BUTTON_MIDDLE,
    };

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

const struct mouse_state *mouse_get_state(void)
{
    return &state;
}
