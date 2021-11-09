/**
 * @file byte_ring_buffer.c
 *
 * Ring buffer of bytes implementation
 *
 * @author German Rivera
 */
#include "byte_ring_buffer.h"
#include "runtime_checks.h"
#include "rtos_wrapper.h"
#include "atomic_utils.h"

/**
 * Initialize a byte ring buffer
 *
 * @param ring_buffer_p pointer to the ring buffer
 */
void byte_ring_buffer_init(struct byte_ring_buffer *ring_buffer_p,
                           uint8_t *data_area_p,
                           uint16_t num_entries)
{
    D_ASSERT(data_area_p != NULL);
    D_ASSERT(num_entries != 0);

    ring_buffer_p->signature = BYTE_RING_BUFF_SIGNATURE;
    ring_buffer_p->data_area_p = data_area_p;
    ring_buffer_p->data_area_end_p = data_area_p + num_entries;
    ring_buffer_p->write_cursor_p = data_area_p;
    ring_buffer_p->read_cursor_p = data_area_p;
    ring_buffer_p->num_entries = num_entries;
    ring_buffer_p->num_entries_filled = 0;
    rtos_semaphore_init(&ring_buffer_p->producer_semaphore, "ring buffer producer semaphore",
                        num_entries);
    rtos_semaphore_init(&ring_buffer_p->consumer_semaphore, "ring buffer consumer semaphore",
                        0);
}


/**
 * Write to next available entry in the given byte ring buffer.
 * If it is full, the caller will block until an entry becomes available.
 *
 * @param ring_buffer_p pointer to the byte ring buffer
 * @param byte value to write to the ring buffer
 */
void byte_ring_buffer_write(struct byte_ring_buffer *ring_buffer_p, uint8_t byte)
{
    D_ASSERT(ring_buffer_p->signature == BYTE_RING_BUFF_SIGNATURE);
    D_ASSERT(ring_buffer_p->num_entries_filled <= ring_buffer_p->num_entries);

    rtos_semaphore_wait(&ring_buffer_p->producer_semaphore);
    D_ASSERT(ring_buffer_p->num_entries_filled < ring_buffer_p->num_entries);

    uint32_t old_primask = disable_cpu_interrupts();

    *ring_buffer_p->write_cursor_p++ = byte;
    if (ring_buffer_p->write_cursor_p == ring_buffer_p->data_area_end_p) {
        ring_buffer_p->write_cursor_p = ring_buffer_p->data_area_p;
    }

    ring_buffer_p->num_entries_filled ++;

    restore_cpu_interrupts(old_primask);
    rtos_semaphore_signal(&ring_buffer_p->consumer_semaphore);
}


/**
 * Write to next available entry in the given byte ring buffer, if
 * there is room in the buffer.
 *
 * @param ring_buffer_p pointer to the byte ring buffer
 * @param byte value to write to the ring buffer
 *
 * @return true, if success
 * @return false, if buffer full
 */
bool byte_ring_buffer_write_non_blocking(struct byte_ring_buffer *ring_buffer_p,
                                         uint8_t byte)
{
    D_ASSERT(ring_buffer_p->signature == BYTE_RING_BUFF_SIGNATURE);
    D_ASSERT(ring_buffer_p->num_entries_filled <= ring_buffer_p->num_entries);

    uint32_t old_primask = disable_cpu_interrupts();

    if (ring_buffer_p->num_entries_filled == ring_buffer_p->num_entries) {
        restore_cpu_interrupts(old_primask);
        return false;
    }

    *ring_buffer_p->write_cursor_p++ = byte;
    if (ring_buffer_p->write_cursor_p == ring_buffer_p->data_area_end_p) {
        ring_buffer_p->write_cursor_p = ring_buffer_p->data_area_p;
    }

    ring_buffer_p->num_entries_filled ++;

    restore_cpu_interrupts(old_primask);
    rtos_semaphore_signal(&ring_buffer_p->consumer_semaphore);
    return true;
}


/**
 * Read to next filled entry in the given byte ring buffer, if the ring buffer
 * is not empty. Otherwise, it blocks the caller until the ring buffer becomes
 * not empty.
 *
 * @param ring_buffer_p pointer to the byte ring buffer
 *
 * @return byte value read from the ring buffer
 *
 */
uint8_t byte_ring_buffer_read(struct byte_ring_buffer *ring_buffer_p)
{
    D_ASSERT(ring_buffer_p->signature == BYTE_RING_BUFF_SIGNATURE);
    D_ASSERT(ring_buffer_p->num_entries_filled <= ring_buffer_p->num_entries);

    rtos_semaphore_wait(&ring_buffer_p->consumer_semaphore);
    D_ASSERT(ring_buffer_p->num_entries_filled != 0);

    uint32_t old_primask = disable_cpu_interrupts();

    uint8_t byte = *ring_buffer_p->read_cursor_p++;
    if (ring_buffer_p->read_cursor_p == ring_buffer_p->data_area_end_p) {
        ring_buffer_p->read_cursor_p = ring_buffer_p->data_area_p;
    }

    ring_buffer_p->num_entries_filled --;

    restore_cpu_interrupts(old_primask);
    rtos_semaphore_signal(&ring_buffer_p->producer_semaphore);
    return byte;
}
