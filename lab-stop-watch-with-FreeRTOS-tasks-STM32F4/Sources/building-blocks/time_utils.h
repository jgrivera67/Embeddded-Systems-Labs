/**
 * @file time_utils.h
 *
 * Time utilities interface
 *
 * @author: German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_TIME_UTILS_H_
#define SOURCES_BUILDING_BLOCKS_TIME_UTILS_H_

#include <stdint.h>
#include <stdint.h>
#include "microcontroller.h"

/**
 * Maximum value that can be stored in the SysTick's reload
 * register
 */
#define SYSTICK_MAX_RELOAD_VALUE	UINT32_C(0xffffff)

/**
 * Convert from microseconds to CPU clock cycles
 */
#define MICROSECONDS_TO_CPU_CLOCK_CYCLES(_micro_secs) \
        ((uint32_t)(_micro_secs) * CPU_CLOCK_FREQ_MHZ)

/**
 * Convert from CPU clock cycles to microseconds
 */
#define CPU_CLOCK_CYCLES_TO_MICROSECONDS(_cycles) \
        ((_cycles) / MCU_CPU_CLOCK_FREQ_IN_MHZ)

/**
 * Convert from milliseconds to CPU clock cycles
 */
#define MILLISECONDS_TO_CPU_CLOCK_CYCLES(_milli_secs) \
        (((uint32_t)(_milli_secs) * 1000) * CPU_CLOCK_FREQ_MHZ)

/**
 * Convert from CPU clock cycles to milliseconds
 */
#define CPU_CLOCK_CYCLES_TO_MILLISECONDS(_cycles) \
        (CPU_CLOCK_CYCLES_TO_MICROSECONDS(_cycles) / 1000)


void init_cpu_clock_cycles_counter(void);

uint32_t cpu_clock_cycles_diff(uint32_t begin_cycles, uint32_t end_cycles);

void delay_us(uint32_t us);

/**
 * Get the current value of the CPU clock cycle counter
 * NOTE: SysTick counts backwards
 *
 * @pre Before this macro is invoked for the first time,
 *      init_cpu_clock_cycles_counter() needs to be called.
 */
#if 0 /*FIXME: FreeRTOS uses SysTick as the OS tick timer */
#define get_cpu_clock_cycles() \
		(SYSTICK_MAX_RELOAD_VALUE - SysTick->VAL)
#else
#define get_cpu_clock_cycles() 0
#endif

#endif /* SOURCES_BUILDING_BLOCKS_TIME_UTILS_H_ */
