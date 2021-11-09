/**
 * @file watchdog.c
 *
 * Hardware watchdog timer driver implementation
 *
 * @author German Rivera
 */
#include "watchdog.h"
#include "io_utils.h"
#include "atomic_utils.h"
#include <MK64F12.h>

/**
 * Watchdog control block type
 */
struct watchdog {
    /**
     * Bit vector of expected liveness events
     */
    volatile uint32_t expected_liveness_events;

    /**
     * Bit vector of signaled liveness events
     */
    volatile uint32_t signaled_liveness_events;

    /**
     * Value of expected_liveness_events before the last CPU reset
     * triggered by the watchdog timing out.
     */
    uint32_t old_expected_liveness_events;

    /**
     * Value of signaled_liveness_events before the last CPU reset
     * triggered by the watchdog timing out.
     */
    uint32_t old_signaled_liveness_events;
};

/**
 * Watchdog control block
 *
 * NOTE: This is not a regular C global variable, as it is not
 * in the '.data' section nor in  the '.bss' section (see linker script).
 */
static struct watchdog
    __attribute__ ((section(".watchdog"))) g_watchdog;


/**
 * Initializes watchdog timer
 */
void watchdog_init(void)
{
    uint32_t reg_value;

    /*
     * If the last CPU reset was caused by the watchdog, save the
     * liveness events masks from before the reset.
     */
    reg_value = READ_MMIO_REGISTER(&RCM->SRS0);
    if (reg_value & RCM_SRS0_WDOG_MASK) {
        g_watchdog.old_expected_liveness_events = g_watchdog.expected_liveness_events;
        g_watchdog.old_signaled_liveness_events = g_watchdog.signaled_liveness_events;
    } else {
        g_watchdog.old_expected_liveness_events = 0x0;
        g_watchdog.old_signaled_liveness_events = 0x0;
    }

    g_watchdog.expected_liveness_events = 0x0;
    g_watchdog.signaled_liveness_events = 0x0;

    /*
     * NOTE: First, we need to unlock the Watchdog, and to do so, two
     * writes must be done on the 'WDOG->UNLOCK' register without using
     * I/O accessors, due to strict timing requirements.
     */

    uint32_t int_mask = disable_cpu_interrupts();

    WDOG->UNLOCK = WDOG_UNLOCK_WDOGUNLOCK(0xC520); /* Key 1 */
    WDOG->UNLOCK = WDOG_UNLOCK_WDOGUNLOCK(0xD928); /* Key 2 */

    restore_cpu_interrupts(int_mask);

    /*
     * Select LPO as clock source for the watchdog:
     */
    reg_value = READ_MMIO_REGISTER(&WDOG_STCTRLH);
    reg_value &= ~WDOG_STCTRLH_CLKSRC_MASK;
    WRITE_MMIO_REGISTER(&WDOG_STCTRLH, reg_value);
}


/**
 * Restarts the watchdog timer if all the expected liveness events have
 * been signaled.
 *
 * NOTE: This function is to be invoked from the idle task.
 *
 * GERMAN: This implementation is not very robust as it will not detect if all tasks
 * (except the idle task) are stuck, since if no task is runnable, then the
 * idle task still can run as long as the timer interrupt is still firing.
 */
void watchdog_restart(void)
{
    if ((g_watchdog.signaled_liveness_events & g_watchdog.expected_liveness_events) !=
        g_watchdog.expected_liveness_events) {
        return;
    }

    g_watchdog.signaled_liveness_events = 0x0;

    uint32_t int_mask = disable_cpu_interrupts();

    /*
     * Write the "watchdog refresh" sequence to Watchdog refresh register:
     *
     * NOTE: WRITE_MMIO_REGISTER() is not used due to strict timing
     * requirements (second write must happen within 20 bus clock cycles
     * of the first write).
     */
    WDOG->REFRESH = 0xA602;
    WDOG->REFRESH = 0xB480;

    restore_cpu_interrupts(int_mask);
}


/**
 * Registers a new expected liveness event
 *
 * @param event_mask Liveness event mask
 */
void watchdog_expect_liveness_event(uint32_t event_mask)
{
    D_ASSERT((g_watchdog.expected_liveness_events & event_mask) == 0);
    g_watchdog.expected_liveness_events |= event_mask;
}


/**
 * Notifies that a given liveness event has happened
 *
 * @param event_mask Liveness event mask
 */
void watchdog_signal_liveness_event(uint32_t event_mask)
{
    D_ASSERT((g_watchdog.expected_liveness_events & event_mask) != 0);
    g_watchdog.signaled_liveness_events |= event_mask;
}


void watchdog_get_before_reset_info(uint32_t *old_expected_liveness_events_p,
                                    uint32_t *old_signaled_liveness_events_p)
{
    *old_expected_liveness_events_p = g_watchdog.old_expected_liveness_events;
    *old_signaled_liveness_events_p = g_watchdog.old_signaled_liveness_events;
}
