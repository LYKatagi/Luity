#include <drivers/input/ps2/controller.h>
#include <interrupts/irq12.h>
#include <interrupts/pic.h>

static irq12_handler_t irq12_handler;

bool irq12_register_handler(irq12_handler_t handler)
{
    if (handler == 0) {
        return false;
    }

    irq12_handler = handler;
    return true;
}

bool irq12_enable(void)
{
    if (irq12_handler == 0) {
        return false;
    }

    return pic_enable_irq(PIC_IRQ12);
}

void irq12_disable(void)
{
    pic_disable_irq(PIC_IRQ12);
}

void irq12_dispatch(void)
{
    uint8_t status = ps2_read_status();
    uint8_t data;

    if ((status & PS2_STATUS_OUTPUT_FULL) != 0) {
        data = ps2_read_data();

        if ((status & PS2_STATUS_AUXILIARY) != 0 &&
            (status & (PS2_STATUS_TIMEOUT | PS2_STATUS_PARITY)) == 0 &&
            irq12_handler != 0) {
            irq12_handler(data);
        }
    }

    pic_send_eoi(PIC_IRQ12);
}
