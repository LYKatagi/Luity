#ifndef LUITY_INTERRUPTS_PIC_H
#define LUITY_INTERRUPTS_PIC_H

#include <stdbool.h>

#define PIC_IRQ12 12

void pic_init(void);
bool pic_enable_irq(unsigned int irq);
void pic_disable_irq(unsigned int irq);
void pic_send_eoi(unsigned int irq);

#endif
