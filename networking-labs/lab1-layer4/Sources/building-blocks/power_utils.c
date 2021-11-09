/**
 * @file power_utils.c
 *
 * Power utilities implementation
 *
 * @author German Rivera
 */

#include "power_utils.h"
#include "io_utils.h"
#include "microcontroller.h"

static volatile bool g_sleep_deep_enabled = false;

/**
 * Stops the calling CPU core, putting it in deep sleep mode
 */
void stop_cpu(void)
{
	uint32_t reg_value;

	if (g_sleep_deep_enabled) {
        /*
         * Enable deep sleep mode (stop mode) in the ARM Cortex-M core:
         */
        reg_value = READ_MMIO_REGISTER(&SCB->SCR);
        reg_value |= SCB_SCR_SLEEPDEEP_Msk;
        WRITE_MMIO_REGISTER(&SCB->SCR, reg_value);
	}

	/*
	 * Go to deep sleep mode:
	 */
	__WFI();
}


void enable_deep_sleep(void)
{
	g_sleep_deep_enabled = true;
}
