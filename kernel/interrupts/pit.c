#include <interrupts/pit.h>
#include <interrupts/pic.h>
#include <io.h>

#define PIT_CHANNEL_0 0x40
#define PIT_COMMAND   0x43
#define PIT_BASE_HZ   1193182u

static volatile uint64_t ticks;
static uint32_t current_frequency;

uint64_t pit_ticks(void)
{
    return ticks;
}

void pit_tick(void)
{
    ticks++;
}

void pit_delay_ms(uint32_t milliseconds)
{
    uint64_t wait_ticks = ((uint64_t)milliseconds * current_frequency + 999) / 1000;
    uint64_t deadline = ticks + wait_ticks;
    while (ticks < deadline) {
        __asm__ volatile("hlt" : : : "memory");
    }
}

bool pit_init(uint32_t frequency_hz)
{
    if (frequency_hz == 0) {
        return false;
    }

    current_frequency = frequency_hz;
    ticks = 0;

    uint32_t divisor = PIT_BASE_HZ / frequency_hz;
    if (divisor == 0 || divisor > 0xFFFF) {
        return false;
    }

    /* Channel 0, low/high divisor bytes, square-wave mode, binary counter. */
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL_0, (uint8_t)divisor);
    outb(PIT_CHANNEL_0, (uint8_t)(divisor >> 8));

    return pic_enable_irq(0);
}
