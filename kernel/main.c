#include <graphics.h>
#include <limine_framebuffer.h>

static void halt(void)
{
    for (;;) {
        __asm__ volatile("hlt");
    }
}

void kmain(void)
{
    static const struct gfx_color background = {18, 24, 38};
    static const struct gfx_color panel = {35, 48, 67};
    static const struct gfx_color accent = {75, 190, 170};
    static const struct gfx_color text = {235, 240, 248};
    struct gfx_context graphics;

    if (!limine_framebuffer_init(&graphics)) {
        halt();
    }

    gfx_clear(&graphics, background);
    gfx_fill_rect(&graphics, 24, 24, 300, 72, panel);
    gfx_draw_rect(&graphics, 23, 23, 302, 74, accent);
    gfx_draw_text(&graphics, 36, 34, "Luity Kernel", text, 0);
    gfx_draw_text(&graphics, 36, 58, "Boot by Limine & First init", text, 0);

    halt();
}
