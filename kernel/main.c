#include <limine.h>
#include <stdbool.h>
#include <stdint.h>

__attribute__((used, section(".limine_requests_start"), aligned(8)))
static volatile uint64_t limine_requests_start_marker[] =
    LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests"), aligned(8)))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests"), aligned(8)))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0,
    .response = 0,
};

__attribute__((used, section(".limine_requests_end"), aligned(8)))
static volatile uint64_t limine_requests_end_marker[] =
    LIMINE_REQUESTS_END_MARKER;

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

static bool pack_rgb(const struct limine_framebuffer *fb,
                     uint8_t red, uint8_t green, uint8_t blue,
                     uint32_t *pixel)
{
    if (fb->memory_model != LIMINE_FRAMEBUFFER_RGB ||
        (fb->bpp != 24 && fb->bpp != 32) ||
        !valid_mask(fb->red_mask_size, fb->red_mask_shift, fb->bpp) ||
        !valid_mask(fb->green_mask_size, fb->green_mask_shift, fb->bpp) ||
        !valid_mask(fb->blue_mask_size, fb->blue_mask_shift, fb->bpp)) {
        return false;
    }

    *pixel = scale_channel(red, fb->red_mask_size, fb->red_mask_shift) |
             scale_channel(green, fb->green_mask_size, fb->green_mask_shift) |
             scale_channel(blue, fb->blue_mask_size, fb->blue_mask_shift);
    return true;
}

static void fill_rect(const struct limine_framebuffer *fb,
                      uint64_t x, uint64_t y, uint64_t width, uint64_t height,
                      uint8_t red, uint8_t green, uint8_t blue)
{
    uint32_t pixel;
    uint64_t bytes_per_pixel = fb->bpp / 8;

    if (!pack_rgb(fb, red, green, blue, &pixel) ||
        x >= fb->width || y >= fb->height) {
        return;
    }

    if (width > fb->width - x) {
        width = fb->width - x;
    }
    if (height > fb->height - y) {
        height = fb->height - y;
    }

    for (uint64_t row = 0; row < height; row++) {
        volatile uint8_t *destination =
            (uint8_t *)fb->address + (y + row) * fb->pitch +
            x * bytes_per_pixel;

        for (uint64_t column = 0; column < width; column++) {
            for (uint64_t byte = 0; byte < bytes_per_pixel; byte++) {
                destination[column * bytes_per_pixel + byte] =
                    (uint8_t)(pixel >> (byte * 8));
            }
        }
    }
}

static bool framebuffer_is_usable(const struct limine_framebuffer *fb)
{
    uint64_t bytes_per_pixel;

    if (fb == 0 || fb->address == 0 || fb->width == 0 || fb->height == 0 ||
        fb->pitch == 0 || (fb->bpp != 24 && fb->bpp != 32)) {
        return false;
    }

    bytes_per_pixel = fb->bpp / 8;
    return fb->width <= fb->pitch / bytes_per_pixel &&
           fb->height <= UINT64_MAX / fb->pitch &&
           pack_rgb(fb, 0, 0, 0, &(uint32_t){0});
}

static void draw_test_pattern(const struct limine_framebuffer *fb)
{
    uint64_t bar_width = fb->width / 6;
    uint64_t bar_height = fb->height / 3;
    uint64_t gap = bar_width / 4;
    uint64_t group_width = bar_width * 3 + gap * 2;
    uint64_t x = (fb->width - group_width) / 2;
    uint64_t y = (fb->height - bar_height) / 2;

    fill_rect(fb, 0, 0, fb->width, fb->height, 18, 24, 38);
    fill_rect(fb, x, y, bar_width, bar_height, 255, 32, 32);
    fill_rect(fb, x + bar_width + gap, y, bar_width, bar_height,
              32, 255, 32);
    fill_rect(fb, x + (bar_width + gap) * 2, y, bar_width, bar_height,
              32, 96, 255);
}

static void halt(void)
{
    for (;;) {
        __asm__ volatile("hlt");
    }
}

void kmain(void)
{
    struct limine_framebuffer_response *response = framebuffer_request.response;

    if (limine_base_revision[2] != 0 || response == 0 ||
        response->framebuffer_count == 0 ||
        response->framebuffers == 0 || response->framebuffers[0] == 0 ||
        !framebuffer_is_usable(response->framebuffers[0])) {
        halt();
    }

    draw_test_pattern(response->framebuffers[0]);
    halt();
}
