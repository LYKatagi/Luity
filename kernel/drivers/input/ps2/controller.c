#include <drivers/input/ps2/controller.h>
#include <io.h>


static bool wait_input(void)
{
    for (uint32_t timeout = 0; timeout < PS2_POLL_LIMIT; timeout++) {
        if ((inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_FULL) == 0) {
            return true;
        }
    }

    return false;
}

static bool wait_output(void)
{
    for (uint32_t timeout = 0; timeout < PS2_POLL_LIMIT; timeout++) {
        if ((inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) != 0) {
            return true;
        }
    }

    return false;
}

bool ps2_wait_input(void)
{
    return wait_input();
}

bool ps2_wait_output(void)
{
    return wait_output();
}

uint8_t ps2_read_data(void)
{
    return inb(PS2_DATA_PORT);
}

uint8_t ps2_read_status(void)
{
    return inb(PS2_STATUS_PORT);
}

bool ps2_try_read_data(uint8_t *data)
{
    if (data == 0 || !wait_output()) {
        return false;
    }

    uint8_t status = ps2_read_status();
    *data = ps2_read_data();

    /* Consume errored bytes too, but report them to the caller as invalid. */
    return (status & (PS2_STATUS_TIMEOUT | PS2_STATUS_PARITY)) == 0;
}

bool ps2_write_data(uint8_t data)
{
    if (!wait_input()) {
        return false;
    }

    outb(PS2_DATA_PORT, data);
    return true;
}

bool ps2_write_command(uint8_t command)
{
    if (!wait_input()) {
        return false;
    }

    outb(PS2_COMMAND_PORT, command);
    return true;
}

bool ps2_write_mouse(uint8_t data)
{
    if (!ps2_write_command(0xD4)) {
        return false;
    }

    /* 0xD4 routes the next data byte to the auxiliary device. */
    return ps2_write_data(data);
}

static bool flush_output_buffer(void)
{
    /* Discard stale keyboard bytes before reading the controller config byte. */
    for (uint32_t count = 0; count < 64; count++) {
        if ((ps2_read_status() & PS2_STATUS_OUTPUT_FULL) == 0) {
            return true;
        }
        (void)ps2_read_data();
    }

    return (ps2_read_status() & PS2_STATUS_OUTPUT_FULL) == 0;
}

bool ps2_enable_auxiliary_irq(void)
{
    uint8_t config;

    /* Quiesce the second port and discard stale data before reading config. */
    if (!ps2_write_command(0xA7)) {
        return false;
    }
    if (!flush_output_buffer()) {
        return false;
    }

    if (!ps2_write_command(0x20) || !ps2_wait_output()) {
        return false;
    }

    uint8_t status = ps2_read_status();
    config = ps2_read_data();
    if ((status & (PS2_STATUS_AUXILIARY | PS2_STATUS_TIMEOUT |
                   PS2_STATUS_PARITY)) != 0) {
        return false;
    }

    config = (uint8_t)((config | 0x02) & ~0x20);

    if (!ps2_write_command(0x60) || !ps2_write_data(config) ||
        !ps2_write_command(0xA8)) {
        return false;
    }

    return true;
}
