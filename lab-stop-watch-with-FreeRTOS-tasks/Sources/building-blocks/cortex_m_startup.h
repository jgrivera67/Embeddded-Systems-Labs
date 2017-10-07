/**
 * @file cortex_m_startup.h
 *
 * Startup-related declarations for ARM Cortex-M processors
 *
 * @author: German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_CORTEX_M_STARTUP_H_
#define SOURCES_BUILDING_BLOCKS_CORTEX_M_STARTUP_H_

#include <stdint.h>

uint32_t get_starup_time_us(void);

uint32_t get_flash_used(void);

uint32_t get_sram_used(void);

/*
 * Variable representing the bottom of the stack used by the reset exception
 * handler.
 * It is defined in the linker script  as __StackTop as it corresponds to the
 *  initial top of the stack upon reset.
 */
extern uint32_t __StackTop[];

/*
 * Variable representing the top limit of the stack used by the reset exception
 * handler.
 * It is defined in the linker script  as __StackLimit.
 */
extern uint32_t __StackLimit[];

#endif /* SOURCES_BUILDING_BLOCKS_CORTEX_M_STARTUP_H_ */
