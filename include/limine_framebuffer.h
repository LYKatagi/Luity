#ifndef LUITY_LIMINE_FRAMEBUFFER_H
#define LUITY_LIMINE_FRAMEBUFFER_H

#include <stdbool.h>

#include <graphics.h>

/* Adapts Limine's first RGB framebuffer to the generic graphics API. */
bool limine_framebuffer_init(struct gfx_context *context);

#endif
