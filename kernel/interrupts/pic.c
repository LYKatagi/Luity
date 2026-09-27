#include <interrupts/pic.h>
#include <io.h>
#include <stdint.h>

#define PIC_MASTER_COMMAND 0x20
#define PIC_MASTER_DATA    0x21
#define PIC_SLAVE_COMMAND  0xA0
#define PIC_SLAVE_DATA     0xA1
#define PIC_EOI            0x20

static void io_wait(void)
{
    outb(0x80, 0);
}

static void write_masks(uint8_t master, uint8_t slave)
{
    outb(PIC_MASTER_DATA, master);
    outb(PIC_SLAVE_DATA, slave);
}

void pic_init(void)
{
    __asm__ volatile("cli" : : : "memory");

    /* Keep every IRQ masked while the IDT and device handlers are installed. */
    write_masks(0xFF, 0xFF);

    outb(PIC_MASTER_COMMAND, 0x11);
    io_wait();
    outb(PIC_SLAVE_COMMAND, 0x11);
    io_wait();

    outb(PIC_MASTER_DATA, 0x20);
    io_wait();
    outb(PIC_SLAVE_DATA, 0x28);
    io_wait();

    outb(PIC_MASTER_DATA, 0x04);
    io_wait();
    outb(PIC_SLAVE_DATA, 0x02);
    io_wait();

    outb(PIC_MASTER_DATA, 0x01);
    io_wait();
    outb(PIC_SLAVE_DATA, 0x01);
    io_wait();

    write_masks(0xFF, 0xFF);
}

bool pic_enable_irq(unsigned int irq)
{
    if (irq >= 16) {
        return false;
    }

    uint16_t port = irq < 8 ? PIC_MASTER_DATA : PIC_SLAVE_DATA;
    unsigned int line = irq < 8 ? irq : irq - 8;
    uint8_t mask = inb(port);
    outb(port, (uint8_t)(mask & ~(1u << line)));

    /* A slave interrupt also requires the master's cascade input (IRQ2). */
    if (irq >= 8) {
        mask = inb(PIC_MASTER_DATA);
        outb(PIC_MASTER_DATA, (uint8_t)(mask & ~(1u << 2)));
    }

    return true;
}

void pic_disable_irq(unsigned int irq)
{
    if (irq >= 16) {
        return;
    }

    uint16_t port = irq < 8 ? PIC_MASTER_DATA : PIC_SLAVE_DATA;
    unsigned int line = irq < 8 ? irq : irq - 8;
    uint8_t mask = inb(port);
    outb(port, (uint8_t)(mask | (1u << line)));

    if (irq >= 8) {
        uint8_t slave_mask = inb(PIC_SLAVE_DATA);
        if (slave_mask == 0xFF) {
            uint8_t master_mask = inb(PIC_MASTER_DATA);
            outb(PIC_MASTER_DATA, (uint8_t)(master_mask | (1u << 2)));
        }
    }
}

void pic_send_eoi(unsigned int irq)
{
    if (irq >= 8 && irq < 16) {
        outb(PIC_SLAVE_COMMAND, PIC_EOI);
    }

    if (irq < 16) {
        outb(PIC_MASTER_COMMAND, PIC_EOI);
    }
}
