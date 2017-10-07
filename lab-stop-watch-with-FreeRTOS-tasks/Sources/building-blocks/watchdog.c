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
	/*
	 * Disable watchdog timer:
	 */
	  WDOG0->CNT = WDOG_UPDATE_KEY;
	  WDOG0->TOVAL = 0xFFFF;
	  WDOG0->CS = (uint32_t) ((WDOG0->CS) & ~WDOG_CS_EN_MASK) | WDOG_CS_UPDATE_MASK;
}
