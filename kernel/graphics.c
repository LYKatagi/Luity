#include <graphics.h>
#include <font8x8.h>

static bool valid_mask(uint8_t size, uint8_t shift, uint16_t bpp)
{
    return size != 0 && shift < bpp && size <= bpp - shift;
}

static uint32_t scale_channel(uint8_t value, uint8_t size, uint8_t shift)
{
    uint64_t max_value = (1ULL << size) - 1;
    uint64_t scaled = ((uint64_t)value * max_value + 127) / 255;

    return (uint32_t)(scaled << shift);
}

static uint64_t channel_mask(uint8_t size, uint8_t shift)
{
    return ((1ULL << size) - 1) << shift;
}

static bool pack_color(const struct gfx_surface *surface,
                       struct gfx_color color, uint32_t *pixel)
{
    if (surface->format != GFX_PIXEL_FORMAT_RGB ||
        (surface->bits_per_pixel != 24 && surface->bits_per_pixel != 32) ||
        !valid_mask(surface->red_mask_size, surface->red_mask_shift,
                    surface->bits_per_pixel) ||
        !valid_mask(surface->green_mask_size, surface->green_mask_shift,
                    surface->bits_per_pixel) ||
        !valid_mask(surface->blue_mask_size, surface->blue_mask_shift,
                    surface->bits_per_pixel)) {
        return false;
    }

    uint64_t red_mask =
        channel_mask(surface->red_mask_size, surface->red_mask_shift);
    uint64_t green_mask =
        channel_mask(surface->green_mask_size, surface->green_mask_shift);
    uint64_t blue_mask =
        channel_mask(surface->blue_mask_size, surface->blue_mask_shift);
    if ((red_mask & green_mask) != 0 || (red_mask & blue_mask) != 0 ||
        (green_mask & blue_mask) != 0) {
        return false;
    }

    *pixel = scale_channel(color.red, surface->red_mask_size,
                           surface->red_mask_shift) |
             scale_channel(color.green, surface->green_mask_size,
                           surface->green_mask_shift) |
             scale_channel(color.blue, surface->blue_mask_size,
                           surface->blue_mask_shift);
    return true;
}

static void write_packed_pixel(struct gfx_context *context, uint64_t x,
                               uint64_t y, uint32_t pixel)
{
    struct gfx_surface *surface = &context->surface;
    uint64_t bytes_per_pixel = surface->bits_per_pixel / 8;
    volatile uint8_t *destination =
        surface->pixels + y * surface->pitch + x * bytes_per_pixel;

    for (uint64_t byte = 0; byte < bytes_per_pixel; byte++) {
        destination[byte] = (uint8_t)(pixel >> (byte * 8));
    }
}

static bool clip_rect(const struct gfx_surface *surface, uint64_t *x,
                      uint64_t *y, uint64_t *width, uint64_t *height)
{
    if (*width == 0 || *height == 0 ||
        *x >= surface->width || *y >= surface->height) {
        return false;
    }

    if (*width > surface->width - *x) {
        *width = surface->width - *x;
    }
    if (*height > surface->height - *y) {
        *height = surface->height - *y;
    }
    return true;
}

bool gfx_init(struct gfx_context *context, const struct gfx_surface *surface)
{
    uint32_t ignored_pixel;
    uint64_t bytes_per_pixel;

    if (context == 0) {
        return false;
    }
    context->initialized = false;

    if (surface == 0 || surface->pixels == 0 ||
        surface->width == 0 || surface->height == 0 || surface->pitch == 0 ||
        (surface->bits_per_pixel != 24 && surface->bits_per_pixel != 32) ||
        !pack_color(surface, (struct gfx_color){0, 0, 0}, &ignored_pixel)) {
        return false;
    }

    bytes_per_pixel = surface->bits_per_pixel / 8;
    if (surface->width > surface->pitch / bytes_per_pixel ||
        surface->height > UINT64_MAX / surface->pitch) {
        return false;
    }

    context->surface = *surface;
    context->initialized = true;
    return true;
}

uint64_t gfx_width(const struct gfx_context *context)
{
    return context != 0 && context->initialized ? context->surface.width : 0;
}

uint64_t gfx_height(const struct gfx_context *context)
{
    return context != 0 && context->initialized ? context->surface.height : 0;
}

void gfx_clear(struct gfx_context *context, struct gfx_color color)
{
    if (context != 0 && context->initialized) {
        gfx_fill_rect(context, 0, 0, context->surface.width,
                      context->surface.height, color);
    }
}

bool gfx_put_pixel(struct gfx_context *context, uint64_t x, uint64_t y,
                   struct gfx_color color)
{
    uint32_t pixel;

    if (context == 0 || !context->initialized ||
        x >= context->surface.width || y >= context->surface.height ||
        !pack_color(&context->surface, color, &pixel)) {
        return false;
    }

    write_packed_pixel(context, x, y, pixel);
    return true;
}

static uint8_t unpack_channel(uint32_t pixel, uint8_t size, uint8_t shift)
{
    uint64_t max_value = (1ULL << size) - 1;
    uint64_t value = (pixel >> shift) & max_value;

    return (uint8_t)((value * 255 + max_value / 2) / max_value);
}

bool gfx_get_pixel(const struct gfx_context *context, uint64_t x, uint64_t y,
                   struct gfx_color *color)
{
    const struct gfx_surface *surface;
    const volatile uint8_t *source;
    uint64_t bytes_per_pixel;
    uint32_t pixel = 0;

    if (context == 0 || !context->initialized || color == 0 ||
        x >= context->surface.width || y >= context->surface.height) {
        return false;
    }

    surface = &context->surface;
    bytes_per_pixel = surface->bits_per_pixel / 8;
    source = surface->pixels + y * surface->pitch + x * bytes_per_pixel;

    for (uint64_t byte = 0; byte < bytes_per_pixel; byte++) {
        pixel |= (uint32_t)source[byte] << (byte * 8);
    }

    color->red = unpack_channel(pixel, surface->red_mask_size,
                                surface->red_mask_shift);
    color->green = unpack_channel(pixel, surface->green_mask_size,
                                  surface->green_mask_shift);
    color->blue = unpack_channel(pixel, surface->blue_mask_size,
                                 surface->blue_mask_shift);
    return true;
}

void gfx_fill_rect(struct gfx_context *context, uint64_t x, uint64_t y,
                   uint64_t width, uint64_t height, struct gfx_color color)
{
    uint32_t pixel;
    uint64_t bytes_per_pixel;

    if (context == 0 || !context->initialized ||
        !pack_color(&context->surface, color, &pixel) ||
        !clip_rect(&context->surface, &x, &y, &width, &height)) {
        return;
    }

    bytes_per_pixel = context->surface.bits_per_pixel / 8;
    for (uint64_t row = 0; row < height; row++) {
        uint64_t row_offset = (y + row) * context->surface.pitch +
                              x * bytes_per_pixel;
        volatile uint8_t *destination =
            context->surface.pixels + row_offset;

        for (uint64_t column = 0; column < width; column++) {
            uint64_t pixel_offset = column * bytes_per_pixel;
            for (uint64_t byte = 0; byte < bytes_per_pixel; byte++) {
                destination[pixel_offset + byte] =
                    (uint8_t)(pixel >> (byte * 8));
            }
        }
    }
}

void gfx_draw_rect(struct gfx_context *context, uint64_t x, uint64_t y,
                   uint64_t width, uint64_t height, struct gfx_color color)
{
    if (context == 0 || !context->initialized ||
        !clip_rect(&context->surface, &x, &y, &width, &height)) {
        return;
    }

    gfx_fill_rect(context, x, y, width, 1, color);
    if (height > 1) {
        gfx_fill_rect(context, x, y + height - 1, width, 1, color);
    }
    if (height > 2) {
        gfx_fill_rect(context, x, y + 1, 1, height - 2, color);
        if (width > 1) {
            gfx_fill_rect(context, x + width - 1, y + 1, 1,
                          height - 2, color);
        }
    }
}

void gfx_draw_char(struct gfx_context *context, uint64_t x, uint64_t y,
                   uint8_t character, struct gfx_color foreground,
                   const struct gfx_color *background)
{
    uint32_t foreground_pixel;
    uint32_t background_pixel = 0;
    const uint8_t *glyph;

    if (context == 0 || !context->initialized ||
        x > UINT64_MAX - (GFX_FONT_WIDTH - 1) ||
        y > UINT64_MAX - (GFX_FONT_HEIGHT - 1) ||
        !pack_color(&context->surface, foreground, &foreground_pixel) ||
        (background != 0 &&
         !pack_color(&context->surface, *background, &background_pixel))) {
        return;
    }

    glyph = font8x8_get_glyph(character);
    for (uint64_t row = 0; row < GFX_FONT_HEIGHT; row++) {
        for (uint64_t column = 0; column < GFX_FONT_WIDTH; column++) {
            bool set = (glyph[row] & (1u << column)) != 0;
            if (x + column >= context->surface.width ||
                y + row >= context->surface.height) {
                continue;
            }
            if (set) {
                write_packed_pixel(context, x + column, y + row,
                                   foreground_pixel);
            } else if (background != 0) {
                write_packed_pixel(context, x + column, y + row,
                                   background_pixel);
            }
        }
    }
}

void gfx_draw_text(struct gfx_context *context, uint64_t x, uint64_t y,
                   const char *text, struct gfx_color foreground,
                   const struct gfx_color *background)
{
    uint64_t line_start;

    if (context == 0 || !context->initialized || text == 0) {
        return;
    }
    line_start = x;

    for (; *text != '\0'; text++) {
        if (*text == '\n') {
            x = line_start;
            if (y > UINT64_MAX - GFX_FONT_HEIGHT) {
                return;
            }
            y += GFX_FONT_HEIGHT;
        } else if (*text == '\r') {
            x = line_start;
        } else if (*text == '\t') {
            if (x > UINT64_MAX - (GFX_FONT_WIDTH * 4)) {
                return;
            }
            x += GFX_FONT_WIDTH * 4;
        } else {
            if (x >= context->surface.width ||
                (context->surface.width >= GFX_FONT_WIDTH &&
                 context->surface.width - x < GFX_FONT_WIDTH)) {
                x = line_start;
                if (x >= context->surface.width ||
                    context->surface.width - x < GFX_FONT_WIDTH) {
                    x = 0;
                    line_start = 0;
                }
                if (y > UINT64_MAX - GFX_FONT_HEIGHT) {
                    return;
                }
                y += GFX_FONT_HEIGHT;
                if (y >= context->surface.height) {
                    return;
                }
            }

            gfx_draw_char(context, x, y, (uint8_t)*text, foreground,
                          background);
            if (x > UINT64_MAX - GFX_FONT_WIDTH) {
                return;
            }
            x += GFX_FONT_WIDTH;
        }

        if (y >= context->surface.height) {
            return;
        }
    }
}

static void gfx_draw_char_scaled(struct gfx_context *context, uint64_t x,
                                 uint64_t y, uint8_t character,
                                 struct gfx_color foreground,
                                 const struct gfx_color *background,
                                 uint64_t scale)
{
    if (scale == 0 || scale > UINT64_MAX / GFX_FONT_WIDTH) {
        return;
    }

    uint64_t cell_size = GFX_FONT_WIDTH * scale;
    if (x > UINT64_MAX - cell_size || y > UINT64_MAX - cell_size) {
        return;
    }

    if (background != 0) {
        gfx_fill_rect(context, x, y, cell_size, cell_size, *background);
    }

    const uint8_t *glyph = font8x8_get_glyph(character);
    for (uint64_t row = 0; row < GFX_FONT_HEIGHT; row++) {
        for (uint64_t column = 0; column < GFX_FONT_WIDTH; column++) {
            if ((glyph[row] & (1u << column)) != 0) {
                gfx_fill_rect(context, x + column * scale,
                              y + row * scale, scale, scale, foreground);
            }
        }
    }
}

void gfx_draw_text_scaled(struct gfx_context *context, uint64_t x, uint64_t y,
                          const char *text, struct gfx_color foreground,
                          const struct gfx_color *background, uint64_t scale)
{
    if (context == 0 || !context->initialized || text == 0 || scale == 0 ||
        scale > UINT64_MAX / (GFX_FONT_WIDTH * 4)) {
        return;
    }

    uint64_t cell_size = GFX_FONT_WIDTH * scale;
    uint64_t line_start = x;

    for (; *text != '\0'; text++) {
        if (*text == '\n') {
            x = line_start;
            if (y > UINT64_MAX - cell_size) {
                return;
            }
            y += cell_size;
        } else if (*text == '\r') {
            x = line_start;
        } else if (*text == '\t') {
            if (x > UINT64_MAX - cell_size * 4) {
                return;
            }
            x += cell_size * 4;
        } else {
            if (x >= context->surface.width ||
                cell_size > context->surface.width - x) {
                x = line_start;
                if (x >= context->surface.width ||
                    cell_size > context->surface.width - x) {
                    x = 0;
                    line_start = 0;
                }
                if (y > UINT64_MAX - cell_size) {
                    return;
                }
                y += cell_size;
            }

            if (y >= context->surface.height) {
                return;
            }

            gfx_draw_char_scaled(context, x, y, (uint8_t)*text, foreground,
                                 background, scale);
            if (x > UINT64_MAX - cell_size) {
                return;
            }
            x += cell_size;
        }

        if (y >= context->surface.height) {
            return;
        }
    }
}
