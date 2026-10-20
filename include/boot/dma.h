#ifndef LUITY_BOOT_DMA_H
#define LUITY_BOOT_DMA_H

#include <stddef.h>
#include <stdint.h>

/* Allocates contiguous memory below 4 GiB for bus-master DMA. */
void *boot_dma_alloc(size_t size, size_t alignment, uint32_t *physical_address);

#endif
