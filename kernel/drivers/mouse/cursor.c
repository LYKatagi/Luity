#include <stdint.h>

#include <graphics.h>
#include <drivers/mouse/cursor.h>

static const uint8_t cursor_bitmap[MOUSE_CURSOR_HEIGHT][MOUSE_CURSOR_WIDTH] = {
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static struct gfx_color saved_background[MOUSE_CURSOR_RENDER_HEIGHT]
                                         [MOUSE_CURSOR_RENDER_WIDTH];
static uint64_t previous_x;
static uint64_t previous_y;
static uint64_t saved_width;
static uint64_t saved_height;
static bool cursor_visible;

void mouse_cursor_draw(
    struct gfx_context *context,
    uint64_t x,
    uint64_t y
)
{
    const struct gfx_color black = {0, 0, 0};
    const struct gfx_color white = {255, 255, 255};

    if (context == 0 || gfx_width(context) == 0 || gfx_height(context) == 0) {
        return;
    }

    if (cursor_visible) {
        for (uint64_t row = 0; row < saved_height; row++) {
            for (uint64_t column = 0; column < saved_width; column++) {
                gfx_put_pixel(context, previous_x + column, previous_y + row,
                              saved_background[row][column]);
            }
        }
        cursor_visible = false;
    }

    if (x >= gfx_width(context) || y >= gfx_height(context)) {
        return;
    }

    saved_width = gfx_width(context) - x;
    if (saved_width > MOUSE_CURSOR_RENDER_WIDTH) {
        saved_width = MOUSE_CURSOR_RENDER_WIDTH;
    }
    saved_height = gfx_height(context) - y;
    if (saved_height > MOUSE_CURSOR_RENDER_HEIGHT) {
        saved_height = MOUSE_CURSOR_RENDER_HEIGHT;
    }

    for (uint64_t row = 0; row < saved_height; row++) {
        for (uint64_t column = 0; column < saved_width; column++) {
            gfx_get_pixel(context, x + column, y + row,
                          &saved_background[row][column]);
        }
    }

    for (uint64_t row = 0; row < MOUSE_CURSOR_HEIGHT; row++) {
        for (uint64_t column = 0; column < MOUSE_CURSOR_WIDTH; column++) {
            uint8_t pixel = cursor_bitmap[row][column];

            if (pixel == 0) {
                continue;
            }

            uint64_t pixel_x = column * MOUSE_CURSOR_SCALE;
            uint64_t pixel_y = row * MOUSE_CURSOR_SCALE;
            if (pixel_x >= gfx_width(context) - x ||
                pixel_y >= gfx_height(context) - y) {
                continue;
            }

            uint64_t draw_width = MOUSE_CURSOR_SCALE;
            uint64_t draw_height = MOUSE_CURSOR_SCALE;
            if (draw_width > gfx_width(context) - x - pixel_x) {
                draw_width = gfx_width(context) - x - pixel_x;
            }
            if (draw_height > gfx_height(context) - y - pixel_y) {
                draw_height = gfx_height(context) - y - pixel_y;
            }

            gfx_fill_rect(context, x + pixel_x, y + pixel_y,
                          draw_width, draw_height,
                          pixel == 1 ? black : white);
        }
    }

    previous_x = x;
    previous_y = y;
    cursor_visible = true;
}
