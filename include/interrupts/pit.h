#ifndef LUITY_INTERRUPTS_PIT_H
#define LUITY_INTERRUPTS_PIT_H

#include <stdbool.h>
#include <stdint.h>

bool pit_init(uint32_t frequency_hz);
uint64_t pit_ticks(void);
void pit_delay_ms(uint32_t milliseconds);
void pit_tick(void);

#endif
