#ifndef LUITY_FONT8X8_H
#define LUITY_FONT8X8_H

#include <stdint.h>

/* Returns eight row bytes; the least significant bit is the leftmost pixel. */
const uint8_t *font8x8_get_glyph(uint8_t character);

#endif
