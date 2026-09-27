#include <interrupts/idt.h>
#include <stdint.h>

#define IDT_ENTRIES 256
#define IRQ0_VECTOR 0x20
#define IRQ12_VECTOR 0x2C

struct __attribute__((packed)) idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t attributes;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
};

struct __attribute__((packed)) idt_register {
    uint16_t limit;
    uint64_t base;
};

static struct idt_entry idt[IDT_ENTRIES] __attribute__((aligned(16)));

extern void irq12_stub(void);
extern void irq0_stub(void);
extern void interrupt_fatal_stub(void);

static uint16_t read_code_segment(void)
{
    uint16_t selector;
    __asm__ volatile("mov %%cs, %0" : "=r"(selector));
    return selector;
}

static void idt_set_gate(uint8_t vector, void (*handler)(void), uint16_t selector)
{
    uint64_t address = (uint64_t)handler;

    idt[vector].offset_low = (uint16_t)address;
    idt[vector].selector = selector;
    idt[vector].ist = 0;
    idt[vector].attributes = 0x8E; /* Present, ring 0, interrupt gate. */
    idt[vector].offset_middle = (uint16_t)(address >> 16);
    idt[vector].offset_high = (uint32_t)(address >> 32);
    idt[vector].reserved = 0;
}

void idt_init(void)
{
    __asm__ volatile("cli" : : : "memory");
    uint16_t code_segment = read_code_segment();

    for (unsigned int vector = 0; vector < IDT_ENTRIES; vector++) {
        idt_set_gate((uint8_t)vector, interrupt_fatal_stub, code_segment);
    }

    idt_set_gate(IRQ0_VECTOR, irq0_stub, code_segment);
    idt_set_gate(IRQ12_VECTOR, irq12_stub, code_segment);

    struct idt_register idtr = {
        .limit = sizeof(idt) - 1,
        .base = (uint64_t)idt
    };

    __asm__ volatile("lidt %0" : : "m"(idtr) : "memory");
}
