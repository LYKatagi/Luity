#ifndef LUITY_MOUSE_CURSOR_H
#define LUITY_MOUSE_CURSOR_H

#include <stdint.h>

#include <graphics.h>

#define MOUSE_CURSOR_WIDTH  16
#define MOUSE_CURSOR_HEIGHT 16
#define MOUSE_CURSOR_SCALE  2
#define MOUSE_CURSOR_RENDER_WIDTH  (MOUSE_CURSOR_WIDTH * MOUSE_CURSOR_SCALE)
#define MOUSE_CURSOR_RENDER_HEIGHT (MOUSE_CURSOR_HEIGHT * MOUSE_CURSOR_SCALE)

void mouse_cursor_draw(
    struct gfx_context *context,
    uint64_t x,
    uint64_t y
);

#endif
