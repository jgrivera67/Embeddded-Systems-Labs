/**
 * @file watchdog.c
 *
 * Hardware watchdog timer driver
 *
 * @author German Rivera
 */
#include "watchdog.h"
#include "microcontroller.h"

void watchdog_init(void)
{
	uint32_t reg_value;

	/*
	 * Disable watchdog timer:
	 */
	WDOG0->CNT = WDOG_UPDATE_KEY;
	WDOG0->TOVAL = 0xFFFF;
	reg_value = WDOG0->CS;
	reg_value &= ~WDOG_CS_EN_MASK;
	reg_value |= WDOG_CS_UPDATE_MASK;
	WDOG0->CS = reg_value;
}
