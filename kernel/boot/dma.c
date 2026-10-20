#include <boot/dma.h>
#include <limine.h>
#include <stdbool.h>

#define DMA_ADDRESS_LIMIT 0x100000000ull

__attribute__((used, section(".limine_requests"), aligned(8)))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
    .response = 0,
};

__attribute__((used, section(".limine_requests"), aligned(8)))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
    .response = 0,
};

static uint64_t hhdm_offset;
static uint64_t next_physical;
static uint64_t active_region_end;
static uint64_t next_region;

static uint64_t align_up(uint64_t value, uint64_t alignment)
{
    uint64_t mask = alignment - 1;
    if (value > UINT64_MAX - mask) {
        return UINT64_MAX;
    }
    return (value + mask) & ~mask;
}

void *boot_dma_alloc(size_t size, size_t alignment, uint32_t *physical_address)
{
    struct limine_memmap_response *map = memmap_request.response;
    struct limine_hhdm_response *hhdm = hhdm_request.response;
    uint64_t physical;

    if (size == 0 || physical_address == 0 || alignment == 0 ||
        (alignment & (alignment - 1)) != 0 || map == 0 || hhdm == 0) {
        return 0;
    }

    hhdm_offset = hhdm->offset;
    for (;;) {
        physical = align_up(next_physical, alignment);
        if (physical != UINT64_MAX && physical <= active_region_end &&
            size <= active_region_end - physical &&
            physical <= DMA_ADDRESS_LIMIT - size) {
            next_physical = physical + size;
            *physical_address = (uint32_t)physical;
            return (void *)(uintptr_t)(hhdm_offset + physical);
        }

        bool found = false;
        while (next_region < map->entry_count) {
            struct limine_memmap_entry *entry = map->entries[next_region++];
            if (entry == 0 || entry->type != LIMINE_MEMMAP_USABLE ||
                entry->base >= DMA_ADDRESS_LIMIT || entry->length == 0) {
                continue;
            }

            uint64_t end = entry->base + entry->length;
            if (end < entry->base || end > DMA_ADDRESS_LIMIT) {
                end = DMA_ADDRESS_LIMIT;
            }
            next_physical = entry->base;
            active_region_end = end;
            found = true;
            break;
        }

        if (!found) {
            return 0;
        }
    }
}
