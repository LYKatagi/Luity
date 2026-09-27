#ifndef LUITY_GRAPHICS_WIDGETS_BUTTON_H
#define LUITY_GRAPHICS_WIDGETS_BUTTON_H

#include <events/event.h>
#include <graphics.h>

enum gfx_button_state {
    GFX_BUTTON_IDLE,
    GFX_BUTTON_HOVER,
    GFX_BUTTON_PRESSED,
};

enum gfx_button_result {
    GFX_BUTTON_UNCHANGED = 0,
    GFX_BUTTON_REDRAW = 1u << 0,
    GFX_BUTTON_ACTIVATED = 1u << 1,
};

struct gfx_button {
    uint64_t x;
    uint64_t y;
    uint64_t width;
    uint64_t height;
    const char *label;
    uint8_t action;
    enum gfx_button_state state;
    bool armed;
};

void gfx_button_draw(struct gfx_context *graphics,
                     const struct gfx_button *button);
uint8_t gfx_button_handle_event(struct gfx_button *button,
                                const struct kernel_event *event);

#endif
