#include <graphics.h>
#include <drivers/mouse/mouse.h>
#include <drivers/mouse/cursor.h>
#include <splash.h>
#include <drivers/ps2/controller.h>
#include <interrupts/idt.h>
#include <interrupts/irq12.h>
#include <interrupts/pit.h>
#include <interrupts/pic.h>
#include <limine_framebuffer.h>

static void halt(void)
{
    for (;;) {
        __asm__ volatile("hlt");
    }
}

static uint64_t clamp_mouse_position(int64_t position, uint64_t limit,
                                     uint64_t cursor_size)
{
    if (position <= 0 || limit <= cursor_size) {
        return 0;
    }

    uint64_t maximum = limit - cursor_size;
    if ((uint64_t)position >= maximum) {
        return maximum;
    }

    return (uint64_t)position;
}

void kernel_start(void)
{
    struct gfx_context graphics;
    bool mouse_ready = false;

    idt_init();
    pic_init();
    irq12_register_handler(mouse_handle_byte);

    if (ps2_enable_auxiliary_irq() && mouse_init()) {
        mouse_ready = true;
        irq12_enable();
    }

    pit_init(100);

    if (!limine_framebuffer_init(&graphics)) {
        halt();
    }

    mouse_set_bounds(gfx_width(&graphics), gfx_height(&graphics));
    splash_draw(&graphics, mouse_ready);

    const struct mouse_state *mouse = mouse_get_state();
    uint64_t cursor_x = clamp_mouse_position(mouse->x, gfx_width(&graphics),
                                              MOUSE_CURSOR_RENDER_WIDTH);
    uint64_t cursor_y = clamp_mouse_position(mouse->y, gfx_height(&graphics),
                                              MOUSE_CURSOR_RENDER_HEIGHT);
    mouse_cursor_draw(&graphics, cursor_x, cursor_y);

    __asm__ volatile("sti");

    for (;;) {
        int64_t mouse_x;
        int64_t mouse_y;
        uint64_t next_x;
        uint64_t next_y;

        /* Take a consistent snapshot and avoid missing an IRQ while sleeping. */
        __asm__ volatile("cli" : : : "memory");
        mouse = mouse_get_state();
        mouse_x = mouse->x;
        mouse_y = mouse->y;
        next_x = clamp_mouse_position(mouse_x, gfx_width(&graphics),
                                      MOUSE_CURSOR_RENDER_WIDTH);
        next_y = clamp_mouse_position(mouse_y, gfx_height(&graphics),
                                      MOUSE_CURSOR_RENDER_HEIGHT);

        if (next_x == cursor_x && next_y == cursor_y) {
            __asm__ volatile("sti; hlt" : : : "memory");
            continue;
        }

        __asm__ volatile("sti" : : : "memory");
        mouse_cursor_draw(&graphics, next_x, next_y);
        cursor_x = next_x;
        cursor_y = next_y;
    }
}
