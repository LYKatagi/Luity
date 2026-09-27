#ifndef LUITY_EVENTS_EVENT_H
#define LUITY_EVENTS_EVENT_H

#include <stdbool.h>
#include <stdint.h>

enum kernel_event_type {
    KERNEL_EVENT_NONE = 0,
    KERNEL_EVENT_MOUSE_MOVE,
    KERNEL_EVENT_MOUSE_BUTTON_DOWN,
    KERNEL_EVENT_MOUSE_BUTTON_UP,
};

enum mouse_button_mask {
    MOUSE_BUTTON_LEFT = 1u << 0,
    MOUSE_BUTTON_RIGHT = 1u << 1,
    MOUSE_BUTTON_MIDDLE = 1u << 2,
};

struct kernel_event {
    enum kernel_event_type type;
    int64_t x;
    int64_t y;
    uint8_t button;
    uint8_t buttons;
};

/* Producers may call event_post from interrupt context. */
bool event_post(const struct kernel_event *event);
bool event_poll(struct kernel_event *event);

#endif
