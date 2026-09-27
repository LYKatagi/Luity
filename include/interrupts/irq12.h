#ifndef LUITY_INTERRUPTS_IRQ12_H
#define LUITY_INTERRUPTS_IRQ12_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*irq12_handler_t)(uint8_t data);

bool irq12_register_handler(irq12_handler_t handler);
bool irq12_enable(void);
void irq12_disable(void);

/* Called only by the assembly interrupt stub. */
void irq12_dispatch(void);

#endif
