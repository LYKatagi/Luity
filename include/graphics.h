#ifndef LUITY_GRAPHICS_H
#define LUITY_GRAPHICS_H

#include <stdbool.h>
#include <stdint.h>

#define GFX_FONT_WIDTH 8
#define GFX_FONT_HEIGHT 8

enum gfx_pixel_format {
    GFX_PIXEL_FORMAT_RGB = 1,
};

struct gfx_color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct gfx_surface {
    /* RGB byte-addressed pixels; the API supports 24-bit and 32-bit surfaces. */
    volatile uint8_t *pixels;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint16_t bits_per_pixel;
    enum gfx_pixel_format format;
    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
};

struct gfx_context {
    struct gfx_surface surface;
    bool initialized;
};

bool gfx_init(struct gfx_context *context, const struct gfx_surface *surface);
uint64_t gfx_width(const struct gfx_context *context);
uint64_t gfx_height(const struct gfx_context *context);

void gfx_clear(struct gfx_context *context, struct gfx_color color);
bool gfx_put_pixel(struct gfx_context *context, uint64_t x, uint64_t y,
                   struct gfx_color color);
bool gfx_get_pixel(const struct gfx_context *context, uint64_t x, uint64_t y,
                   struct gfx_color *color);
void gfx_fill_rect(struct gfx_context *context, uint64_t x, uint64_t y,
                   uint64_t width, uint64_t height, struct gfx_color color);
void gfx_draw_rect(struct gfx_context *context, uint64_t x, uint64_t y,
                   uint64_t width, uint64_t height, struct gfx_color color);

void gfx_draw_char(struct gfx_context *context, uint64_t x, uint64_t y,
                   uint8_t character, struct gfx_color foreground,
                   const struct gfx_color *background);
/* Text is ASCII; NULL background means transparent glyph pixels. */
void gfx_draw_text(struct gfx_context *context, uint64_t x, uint64_t y,
                   const char *text, struct gfx_color foreground,
                   const struct gfx_color *background);
void gfx_draw_text_scaled(struct gfx_context *context, uint64_t x, uint64_t y,
                          const char *text, struct gfx_color foreground,
                          const struct gfx_color *background, uint64_t scale);

#endif
