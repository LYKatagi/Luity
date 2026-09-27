#ifndef LUITY_UI_SPLASH_H
#define LUITY_UI_SPLASH_H

#include <stdbool.h>
#include <graphics.h>
#include <graphics/widgets/button.h>
#include <events/event.h>

#define SPLASH_BUTTON_COUNT 2

struct splash_screen {
    struct gfx_context *graphics;
    struct gfx_button buttons[SPLASH_BUTTON_COUNT];
    uint64_t message_x;
    uint64_t message_y;
    uint64_t message_width;
};

void splash_init(struct splash_screen *screen, struct gfx_context *graphics,
                 bool mouse_ready);
void splash_handle_event(struct splash_screen *screen,
                         const struct kernel_event *event);

#endif
