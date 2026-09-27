#include <drivers/mouse/mouse.h>
#include <drivers/ps2/controller.h>
#include <interrupts/pic.h>

/* PIT polling keeps input responsive if a platform drops the auxiliary IRQ. */
void irq0_dispatch(void)
{
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
