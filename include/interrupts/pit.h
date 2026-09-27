#ifndef LUITY_INTERRUPTS_PIT_H
#define LUITY_INTERRUPTS_PIT_H

#include <stdbool.h>
#include <stdint.h>

bool pit_init(uint32_t frequency_hz);

#endif
