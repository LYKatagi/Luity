#include <limine.h>
#include <limine_framebuffer.h>

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

bool limine_framebuffer_init(struct gfx_context *context)
{
    struct limine_framebuffer_response *response;
    struct limine_framebuffer *framebuffer;
    struct gfx_surface surface;

    if (context == 0) {
        return false;
    }
    context->initialized = false;

    if (limine_base_revision[2] != 0) {
        return false;
    }

    response = framebuffer_request.response;
    if (response == 0 || response->framebuffer_count == 0 ||
        response->framebuffers == 0 || response->framebuffers[0] == 0) {
        return false;
    }

    framebuffer = response->framebuffers[0];
    if (framebuffer->memory_model != LIMINE_FRAMEBUFFER_RGB) {
        return false;
    }

    surface = (struct gfx_surface) {
        .pixels = framebuffer->address,
        .width = framebuffer->width,
        .height = framebuffer->height,
        .pitch = framebuffer->pitch,
        .bits_per_pixel = framebuffer->bpp,
        .format = GFX_PIXEL_FORMAT_RGB,
        .red_mask_size = framebuffer->red_mask_size,
        .red_mask_shift = framebuffer->red_mask_shift,
        .green_mask_size = framebuffer->green_mask_size,
        .green_mask_shift = framebuffer->green_mask_shift,
        .blue_mask_size = framebuffer->blue_mask_size,
        .blue_mask_shift = framebuffer->blue_mask_shift,
    };

    return gfx_init(context, &surface);
}
