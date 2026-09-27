#ifndef LUITY_MOUSE_H
#define LUITY_MOUSE_H

#include <stdint.h>
#include <stdbool.h>

struct mouse_state {
    int64_t x;
    int64_t y;

    int16_t delta_x;
    int16_t delta_y;

    uint8_t buttons;

    bool left;
    bool right;
    bool middle;
};

bool mouse_init(void);
void mouse_set_bounds(uint64_t width, uint64_t height);

void mouse_handle_byte(uint8_t byte);

const struct mouse_state *mouse_get_state(void);

#endif
