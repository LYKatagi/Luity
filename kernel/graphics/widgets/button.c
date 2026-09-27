#include <graphics/widgets/button.h>

static bool button_contains(const struct gfx_button *button, int64_t x, int64_t y)
{
    if (x < 0 || y < 0 || (uint64_t)x < button->x ||
        (uint64_t)y < button->y) {
        return false;
    }

    return (uint64_t)x - button->x < button->width &&
           (uint64_t)y - button->y < button->height;
}

static uint64_t text_length(const char *text)
{
    uint64_t length = 0;
    if (text == 0) {
        return 0;
    }

    while (length < UINT64_MAX / GFX_FONT_WIDTH && text[length] != '\0') {
        length++;
    }
    return length;
}

void gfx_button_draw(struct gfx_context *graphics,
                     const struct gfx_button *button)
{
    if (graphics == 0 || button == 0 || button->label == 0 ||
        button->width == 0 || button->height == 0) {
        return;
    }

    struct gfx_color fill = {26, 43, 64};
    struct gfx_color border = {53, 78, 104};
    struct gfx_color text = {234, 242, 251};

    if (button->state == GFX_BUTTON_HOVER) {
        fill = (struct gfx_color){31, 72, 82};
        border = (struct gfx_color){63, 221, 198};
    } else if (button->state == GFX_BUTTON_PRESSED) {
        fill = (struct gfx_color){23, 91, 94};
        border = (struct gfx_color){95, 242, 218};
    }

    gfx_fill_rect(graphics, button->x, button->y,
                  button->width, button->height, fill);
    gfx_draw_rect(graphics, button->x, button->y,
                  button->width, button->height, border);

    uint64_t label_width = text_length(button->label) * GFX_FONT_WIDTH;
    uint64_t label_x = button->x + 10;
    if (button->width > label_width + 20) {
        label_x = button->x + (button->width - label_width) / 2;
    }
    uint64_t label_y = button->y +
        (button->height > GFX_FONT_HEIGHT
             ? (button->height - GFX_FONT_HEIGHT) / 2
             : 0);
    gfx_draw_text(graphics, label_x, label_y, button->label, text, 0);
}

uint8_t gfx_button_handle_event(struct gfx_button *button,
                                const struct kernel_event *event)
{
    if (button == 0 || event == 0) {
        return GFX_BUTTON_UNCHANGED;
    }

    enum gfx_button_state old_state = button->state;
    bool inside = button_contains(button, event->x, event->y);
    bool activated = false;

    if (event->type == KERNEL_EVENT_MOUSE_BUTTON_DOWN &&
        event->button == MOUSE_BUTTON_LEFT) {
        button->armed = inside;
        button->state = inside ? GFX_BUTTON_PRESSED : GFX_BUTTON_IDLE;
    } else if (event->type == KERNEL_EVENT_MOUSE_BUTTON_UP &&
               event->button == MOUSE_BUTTON_LEFT) {
        activated = button->armed && inside;
        button->armed = false;
        button->state = inside ? GFX_BUTTON_HOVER : GFX_BUTTON_IDLE;
    } else if (event->type == KERNEL_EVENT_MOUSE_MOVE) {
        if (button->armed && (event->buttons & MOUSE_BUTTON_LEFT) == 0) {
            button->armed = false;
        }
        button->state = button->armed
            ? (inside ? GFX_BUTTON_PRESSED : GFX_BUTTON_IDLE)
            : (inside ? GFX_BUTTON_HOVER : GFX_BUTTON_IDLE);
    }

    uint8_t result = old_state != button->state ? GFX_BUTTON_REDRAW : 0;
    if (activated) {
        result |= GFX_BUTTON_ACTIVATED;
    }
    return result;
}
