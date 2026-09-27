#include <graphics.h>
#include <drivers/input/mouse/mouse.h>
#include <drivers/input/mouse/cursor.h>
#include <drivers/input/ps2/controller.h>
#include <events/event.h>
#include <interrupts/idt.h>
#include <interrupts/irq12.h>
#include <interrupts/pic.h>
#include <interrupts/pit.h>
#include <limine_framebuffer.h>
#include <ui/splash.h>

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
    struct splash_screen splash;
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
    splash_init(&splash, &graphics, mouse_ready);

    const struct mouse_state *mouse = mouse_get_state();
    uint64_t cursor_x = clamp_mouse_position(mouse->x, gfx_width(&graphics),
                                              MOUSE_CURSOR_RENDER_WIDTH);
    uint64_t cursor_y = clamp_mouse_position(mouse->y, gfx_height(&graphics),
                                              MOUSE_CURSOR_RENDER_HEIGHT);
    mouse_cursor_draw(&graphics, cursor_x, cursor_y);

    __asm__ volatile("sti");

    for (;;) {
        struct kernel_event event;

        /* IRQ producers are masked briefly while the event queue is read. */
        __asm__ volatile("cli" : : : "memory");
        if (!event_poll(&event)) {
            __asm__ volatile("sti; hlt" : : : "memory");
            continue;
        }
        __asm__ volatile("sti" : : : "memory");

        mouse_cursor_erase(&graphics);
        splash_handle_event(&splash, &event);
        cursor_x = clamp_mouse_position(event.x, gfx_width(&graphics),
                                        MOUSE_CURSOR_RENDER_WIDTH);
        cursor_y = clamp_mouse_position(event.y, gfx_height(&graphics),
                                        MOUSE_CURSOR_RENDER_HEIGHT);
        mouse_cursor_draw(&graphics, cursor_x, cursor_y);
    }
}
