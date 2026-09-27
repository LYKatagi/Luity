#include <events/event.h>

#define EVENT_QUEUE_CAPACITY 64u
#define EVENT_QUEUE_MASK (EVENT_QUEUE_CAPACITY - 1u)

static struct kernel_event queue[EVENT_QUEUE_CAPACITY];
static volatile uint32_t read_index;
static volatile uint32_t write_index;

bool event_post(const struct kernel_event *event)
{
    if (event == 0 || event->type == KERNEL_EVENT_NONE) {
        return false;
    }

    uint32_t read = read_index;
    uint32_t write = write_index;
    if (write - read >= EVENT_QUEUE_CAPACITY) {
        if (event->type == KERNEL_EVENT_MOUSE_MOVE && write != read) {
            uint32_t last = (write - 1u) & EVENT_QUEUE_MASK;
            if (queue[last].type == KERNEL_EVENT_MOUSE_MOVE) {
                queue[last] = *event;
                return true;
            }
            return false;
        }

        /* Keep the newest click even when stale motion filled the queue. */
        read_index = read + 1u;
    }

    queue[write & EVENT_QUEUE_MASK] = *event;
    __asm__ volatile("" : : : "memory");
    write_index = write + 1u;
    return true;
}

bool event_poll(struct kernel_event *event)
{
    if (event == 0) {
        return false;
    }

    uint32_t read = read_index;
    if (read == write_index) {
        return false;
    }

    __asm__ volatile("" : : : "memory");
    *event = queue[read & EVENT_QUEUE_MASK];
    read_index = read + 1u;
    return true;
}
