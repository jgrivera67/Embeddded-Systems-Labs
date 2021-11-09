/**
 * @file atomic_utils.h
 *
 * Atomic primitives interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_ATOMIC_UTILS_H_
#define SOURCES_BUILDING_BLOCKS_ATOMIC_UTILS_H_

#include <stdint.h>
#include "compile_time_checks.h"

#define ATOMIC_POST_INCREMENT_UINT32(_counter_p) \
        atomic_fetch_add_uint32(_counter_p, 1)

#define ATOMIC_POST_DECREMENT_UINT32(_counter_p) \
        atomic_fetch_sub_uint32(_counter_p, 1)

#define ATOMIC_POST_INCREMENT_UINT16(_counter_p) \
        atomic_fetch_add_uint16(_counter_p, 1)

#define ATOMIC_POST_DECREMENT_UINT16(_counter_p) \
        atomic_fetch_sub_uint16(_counter_p, 1)

#define ATOMIC_POST_INCREMENT_UINT8(_counter_p) \
        atomic_fetch_add_uint8(_counter_p, 1)

#define ATOMIC_POST_DECREMENT_UINT8(_counter_p) \
        atomic_fetch_sub_uint8(_counter_p, 1)

#define ATOMIC_POST_INCREMENT_POINTER(_pointer_p)                       \
        ((void *)atomic_fetch_add_uint32(                               \
                    (uint32_t *)&(_pointer_p), sizeof(*(_pointer_p))))

C_ASSERT(sizeof(void *) == sizeof(uint32_t));

uint32_t disable_cpu_interrupts(void);

void restore_cpu_interrupts(uint32_t old_primask);

 void get_max_interrupts_disabled_stats_us(uint32_t *max_time_us, uintptr_t *code_addr);

uint32_t atomic_fetch_add_uint32(volatile uint32_t *counter_p, uint32_t value);

uint32_t atomic_fetch_sub_uint32(volatile uint32_t *counter_p, uint32_t value);

uint16_t atomic_fetch_add_uint16(volatile uint16_t *counter_p, uint16_t value);

uint16_t atomic_fetch_sub_uint16(volatile uint16_t *counter_p, uint16_t value);

uint8_t atomic_fetch_add_uint8(volatile uint8_t *counter_p, uint8_t value);

uint8_t atomic_fetch_sub_uint8(volatile uint8_t *counter_p, uint8_t value);

#endif /* SOURCES_BUILDING_BLOCKS_ATOMIC_UTILS_H_ */
