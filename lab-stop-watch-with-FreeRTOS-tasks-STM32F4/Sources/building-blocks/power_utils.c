/**
 * @file power_utils.c
 *
 * Power utilities implementation
 *
 * @author German Rivera
 */

#include "power_utils.h"
#include <building-blocks/io_utils.h>
#include <building-blocks/arm_cmsis.h>

/**
 * Stops the calling CPU core
 */
void stop_cpu(void)
{
	/*
	 * TODO: Add the "wfi" ARM assembly instruction to make the processor
	 * stop executing instruction and resume only if an interrupt occurs.
	 * Hint: see function __WFI() (you can call that function here)
	 */
	__WFI();
}

