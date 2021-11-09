/**
 * @file atomic_utils.c
 *
 * Atomic primitives implementation for ARM Cortex-M
 *
 * @author German Rivera
 */
#include "atomic_utils.h"
#include "microcontroller.h"
#include "time_utils.h"
#include "runtime_checks.h"

/**
 * Interrupts disabled stats variables
 */
struct interrupts_disabled_stats {
    /**
     * Cycles timestamp of last time that interrupts were disabled
     */
    uint32_t start_cycles_interrupts_disabled;

    /**
     * Maximum number of CPU cycles that interrupts have been disabled
     */
    uint32_t max_cycles_interrupts_disabled;

    /**
     * Address in function that has disabled interrupts for the longest time
     */
    uintptr_t longest_interrupts_disabled_code_addr;
};

static struct interrupts_disabled_stats g_interrupts_disabled_stats = {
	.max_cycles_interrupts_disabled = 0,
};

/**
 * Function that disables interrupts and returns the previous interrupt mask
 *
 * @return  Original value of the CPU PRIMASK register
 */
uint32_t
disable_cpu_interrupts(void)
{
    uint32_t old_primask = __get_PRIMASK();

    if (__CPU_INTERRUPTS_ARE_ENABLED(old_primask)) {
        __disable_irq();
        __ISB();

        g_interrupts_disabled_stats.start_cycles_interrupts_disabled = get_cpu_clock_cycles();
    }

    return old_primask;
}


/**
 * Function that restores (and possibly enables) interrupts
 *
 * @param   cpu_status_register: Value of the CPU status register to be
 *          restored
 */
 void
restore_cpu_interrupts(uint32_t old_primask)
{
    void *return_address = __builtin_return_address(0);

    if (__CPU_INTERRUPTS_ARE_ENABLED(old_primask)) {
        uint32_t end_cycles;
        uint32_t delta_cycles;

        end_cycles = get_cpu_clock_cycles();
        delta_cycles = cpu_clock_cycles_diff(g_interrupts_disabled_stats.start_cycles_interrupts_disabled,
        									 end_cycles);
        if (delta_cycles > g_interrupts_disabled_stats.max_cycles_interrupts_disabled) {
        	g_interrupts_disabled_stats.max_cycles_interrupts_disabled = delta_cycles;
        	g_interrupts_disabled_stats.longest_interrupts_disabled_code_addr = GET_CALL_ADDRESS(return_address);
        }

        __ISB();
        __enable_irq();
    }
}


 /**
  * Return maximum time that interrupts have been disabled to date
  */
 void get_max_interrupts_disabled_stats_us(uint32_t *max_time_us, uintptr_t *code_addr)
 {
    *max_time_us = CPU_CLOCK_CYCLES_TO_MICROSECONDS(g_interrupts_disabled_stats.max_cycles_interrupts_disabled);
    *code_addr = g_interrupts_disabled_stats.longest_interrupts_disabled_code_addr;
 }


/**
 * Increments atomically the 32-bit value stored in *counter_p, and returns the
 * original value.
 *
 * @param   counter_p: Pointer to the counter to be incremented.
 *
 * @param   value: Increment value.
 *
 * @return  value of the counter prior to the increment.
 */
uint32_t
atomic_fetch_add_uint32(volatile uint32_t *counter_p, uint32_t value)
{
#if (__CORTEX_M >= 0x03)
    uint32_t old_value;

    do {
    old_value = __LDREXW(counter_p);
    } while (__STREXW(old_value + value, counter_p) != 0);

    return old_value;
#else
    uint32_t old_primask = disable_cpu_interrupts();
    uint32_t old_value = *counter_p;

    *counter_p += value;

    restore_cpu_interrupts(old_primask);
    return old_value;
#endif
}


/**
 * Decrements atomically the 32-bit value stored in *counter_p, and returns the
 * original value.
 *
 * @param   counter_p: Pointer to the counter to be decremented.
 *
 * @param   value: Decrement value.
 *
 * @return  value of the counter prior to the decrement.
 */
uint32_t
atomic_fetch_sub_uint32(volatile uint32_t *counter_p, uint32_t value)
{
#if (__CORTEX_M >= 0x03)
    uint32_t old_value;

    do {
    old_value = __LDREXW(counter_p);
    } while (__STREXW(old_value - value, counter_p) != 0);

    return old_value;
#else
    uint32_t old_primask = disable_cpu_interrupts();
    uint32_t old_value = *counter_p;

    *counter_p -= value;

    restore_cpu_interrupts(old_primask);
    return old_value;
#endif
}


/**
 * Increments atomically the 16-bit value stored in *counter_p, and returns the
 * original value.
 *
 * @param   counter_p: Pointer to the counter to be incremented.
 *
 * @param   value: Increment value.
 *
 * @return  value of the counter prior to the increment.
 */
uint16_t
atomic_fetch_add_uint16(volatile uint16_t *counter_p, uint16_t value)
{
#if (__CORTEX_M >= 0x03)
    uint16_t old_value;

    do {
    old_value = __LDREXH(counter_p);
    } while (__STREXH(old_value + value, counter_p) != 0);

    return old_value;
#else
    uint32_t old_primask = disable_cpu_interrupts();
    uint16_t old_value = *counter_p;

    *counter_p += value;

    restore_cpu_interrupts(old_primask);
    return old_value;
#endif
}


/**
 * Decrements atomically the 16-bit value stored in *counter_p, and returns the
 * original value.
 *
 * @param   counter_p: Pointer to the counter to be decremented.
 *
 * @param   value: Decrement value.
 *
 * @return  value of the counter prior to the decrement.
 */
uint16_t
atomic_fetch_sub_uint16(volatile uint16_t *counter_p, uint16_t value)
{
#if (__CORTEX_M >= 0x03)
    uint16_t old_value;

    do {
    old_value = __LDREXH(counter_p);
    } while (__STREXH(old_value - value, counter_p) != 0);

    return old_value;
#else
    uint32_t old_primask = disable_cpu_interrupts();
    uint16_t old_value = *counter_p;

    *counter_p -= value;

    restore_cpu_interrupts(old_primask);
    return old_value;
#endif
}


/**
 * Increments atomically the 8-bit value stored in *counter_p, and returns the
 * original value.
 *
 * @param   counter_p: Pointer to the counter to be incremented.
 *
 * @param   value: Increment value.
 *
 * @return  value of the counter prior to the increment.
 */
uint8_t
atomic_fetch_add_uint8(volatile uint8_t *counter_p, uint8_t value)
{
#if (__CORTEX_M >= 0x03)
    uint8_t old_value;

    do {
    old_value = __LDREXB(counter_p);
    } while (__STREXB(old_value + value, counter_p) != 0);

    return old_value;
#else
    uint32_t old_primask = disable_cpu_interrupts();
    uint8_t old_value = *counter_p;

    *counter_p += value;

    restore_cpu_interrupts(old_primask);
    return old_value;
#endif
}


/**
 * Decrements atomically the 8-bit value stored in *counter_p, and returns the
 * original value.
 *
 * @param   counter_p: Pointer to the counter to be decremented.
 *
 * @param   value: Decrement value.
 *
 * @return  value of the counter prior to the decrement.
 */
uint8_t
atomic_fetch_sub_uint8(volatile uint8_t *counter_p, uint8_t value)
{
#if (__CORTEX_M >= 0x03)
    uint8_t old_value;

    do {
    old_value = __LDREXB(counter_p);
    } while (__STREXB(old_value - value, counter_p) != 0);

    return old_value;
#else
    uint32_t old_primask = disable_cpu_interrupts();
    uint8_t old_value = *counter_p;

    *counter_p -= value;

    restore_cpu_interrupts(old_primask);
    return old_value;
#endif
}
