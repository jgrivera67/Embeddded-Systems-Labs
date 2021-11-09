/**
 * @file time_utils.c
 *
 * Time utilities implementation
 *
 * @author: German Rivera
 */
#include "time_utils.h"
#include "runtime_checks.h"
#include "microcontroller.h"
#include "atomic_utils.h"


/**
 * Calculate difference between two CPU clock cycle values
 * using n-bit signed two's complement arithmetic, where
 * n is the number of bits of CPU clock cycle values.
 * In this case its 24 bits as that is the width of the Systick's
 * reload register.
 */
#define CPU_CLOCK_CYCLES_DELTA(_begin_cycles, _end_cycles) \
        ((uint32_t)((int32_t)(_end_cycles) - \
                    (int32_t)(_begin_cycles)) & SYSTICK_MAX_RELOAD_VALUE)


/**
 * Initializes counter of CPU clock cycles
 *
 * NOTE This function cannot access any global/static variables,
 * as it is invoked during start-up before  C global/static variables
 * are initialized.
 */
void init_cpu_clock_cycles_counter(void)
{
    /*
     * Configure the system tick timer device (SysTick) for counting CPU clock
     * cycles:
     */

	/*
	 * Set SysTick's reload value using largest value that can fit in 24-bits:
	 */
    SysTick->LOAD = SYSTICK_MAX_RELOAD_VALUE;

    /*
     * Reset SysTick's current value to 0 (by writing any value to it):
     */
    SysTick->VAL = 0;

    /*
     * Select the CPU clock as clock source and start counting:
     */
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk |
                    SysTick_CTRL_CLKSOURCE_Msk;
}


/**
 * Calculates the distance in cycles between two CPU clock cycle count samples,
 * doing a correction to compensate for the overhead of taking the
 * samples themselves
 *
 * @param begin_cycles: start value obtained by calling get_cpu_clockcycles()
 * @param end_cycles: end value obtained by calling get_cpu_clockcycles()
 *
 * @return delta between the two clock cycle count samples
 *
 * NOTE: For more accurate result, interrupts must be disabled in the
 * window between the two samples
 */
uint32_t cpu_clock_cycles_diff(uint32_t begin_cycles, uint32_t end_cycles)
{
	D_ASSERT(begin_cycles != end_cycles);

	return CPU_CLOCK_CYCLES_DELTA(begin_cycles, end_cycles);
}


/**
 * Delays execution a given number of microseconds, using a
 * simple loop that decrements a counter.

 * Each iteration is 3 instructions: sub, cmp and branch.
 * So assuming, 4 CPU clock cycles per iteration (sub 1 cycle,
 * cmp 1 cycle, branch taken 2 cycles) and a clock freq of 48MHz,
 * we can execute 12 iterations in 1 us.
 */
void delay_us(uint32_t us)
{
	uint32_t iterations_per_us = MCU_CPU_CLOCK_FREQ_IN_MHZ / 4;
	register uint32_t n = us * iterations_per_us;

	D_ASSERT(us < 1000);
	D_ASSERT(n / iterations_per_us == us);
	while (n != 0) {
		n--;
	}
}
