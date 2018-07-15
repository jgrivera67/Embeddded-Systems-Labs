/**
 * @file byte_ring_buffer.h
 *
 * Ring buffer of bytes interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BYTE_RING_BUFFER_H_
#define SOURCES_BYTE_RING_BUFFER_H_

#include <stdint.h>
#include "rtos_wrapper.h"

/**
 * Ring buffer of bytes
 */
struct byte_ring_buffer
{
#   define      BYTE_RING_BUFF_SIGNATURE  GEN_SIGNATURE('B', 'R', 'B', 'F')
    uint32_t    signature;

    /**
     * Pointer to data area of the ring buffer. The size of this area
     * must be 'num'_entries' bytes.
     */
    uint8_t *data_area_p;

    /**
      * Pointer to one byte after the last byte of the data area of the ring
      * buffer.
      */
     uint8_t *data_area_end_p;

    /**
     * Pointer to the next entry to fill in the ring buffer
     */
    uint8_t *write_cursor_p;

    /**
     * Pointer to the next entry to read from the ring buffer
     */
    uint8_t *read_cursor_p;

    /**
     * Total number of entries in the ring buffer
     */
    uint16_t num_entries;

    /**
     * Number of entries currently filled in the ring buffer
     */
    uint16_t num_entries_filled;

    /**
     * Producer-side semaphore
     */
    struct rtos_semaphore producer_semaphore;

    /**
     * Consumer-side semaphore
     */
    struct rtos_semaphore consumer_semaphore;
} __attribute__ ((aligned(8*MPU_REGION_ALIGNMENT)));

C_ASSERT(sizeof(struct byte_ring_buffer) == 8*MPU_REGION_ALIGNMENT);

void byte_ring_buffer_init(struct byte_ring_buffer *ring_buffer_p,
                           uint8_t *data_area_p,
                           uint16_t num_entries);

void byte_ring_buffer_write(struct byte_ring_buffer *ring_buffer_p, uint8_t byte);

bool byte_ring_buffer_write_non_blocking(struct byte_ring_buffer *ring_buffer_p,
                                         uint8_t byte);

uint8_t byte_ring_buffer_read(struct byte_ring_buffer *ring_buffer_p);

#endif /* SOURCES_BYTE_RING_BUFFER_H_ */
