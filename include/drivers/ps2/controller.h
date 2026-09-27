#ifndef LUITY_DRIVERS_PS2_CONTROLLER_H
#define LUITY_DRIVERS_PS2_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

/* Ports shared by the first PS/2 port and the auxiliary (mouse) port. */
#define PS2_DATA_PORT       0x60
#define PS2_STATUS_PORT     0x64
#define PS2_COMMAND_PORT    0x64

#define PS2_STATUS_OUTPUT_FULL 0x01
#define PS2_STATUS_INPUT_FULL  0x02
#define PS2_STATUS_AUXILIARY   0x20
#define PS2_STATUS_TIMEOUT     0x40
#define PS2_STATUS_PARITY      0x80

/* Bounds each controller poll so a missing controller cannot hang the kernel. */
#define PS2_POLL_LIMIT 100000u

bool ps2_wait_input(void);
bool ps2_wait_output(void);
uint8_t ps2_read_status(void);
uint8_t ps2_read_data(void);
bool ps2_try_read_data(uint8_t *data);
bool ps2_write_data(uint8_t data);
bool ps2_write_command(uint8_t command);
bool ps2_write_mouse(uint8_t data);
bool ps2_enable_auxiliary_irq(void);

#endif
