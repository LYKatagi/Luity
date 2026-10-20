#include <drivers/input/mouse/mouse.h>
#include <drivers/input/ps2/controller.h>
#include <interrupts/pic.h>
#include <interrupts/pit.h>

/* PIT polling keeps input responsive if a platform drops the auxiliary IRQ. */
void irq0_dispatch(void)
{
    pit_tick();
    uint8_t status = ps2_read_status();

    if ((status & PS2_STATUS_OUTPUT_FULL) != 0) {
        uint8_t data = ps2_read_data();
        if ((status & PS2_STATUS_AUXILIARY) != 0 &&
            (status & (PS2_STATUS_TIMEOUT | PS2_STATUS_PARITY)) == 0) {
            mouse_handle_byte(data);
        }
    }

    pic_send_eoi(0);
}
