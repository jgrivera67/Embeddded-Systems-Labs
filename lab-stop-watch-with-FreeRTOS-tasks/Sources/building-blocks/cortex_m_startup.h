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

/**
 * Variable representing the bottom of the stack used by the reset exception
 * handler.
 * It is defined in the linker script  as __StackTop as it corresponds to the
 *  initial top of the stack upon reset.
 */
extern const uint32_t __StackTop[];

/**
 * Variable representing the top limit of the stack used by the reset exception
 * handler.
 * It is defined in the linker script  as __StackLimit.
 */
extern const uint32_t __StackLimit[];

/**
 * Linker script symbol for the start address of the initialized global variables
 * section
 */
extern const uint32_t __data_start__[];

/**
 * Linker script symbol for the end address of the initialized global variables
 * section
 */
extern const uint32_t __data_end__[];

/**
 * Linker script symbol for the end address of the non-initialized global variables
 * section
 */
extern const uint32_t __bss_end__[];


/**
 * Linker script symbol for the start address of the global flash
 * text region
 */
extern const uint32_t __flash_text_start[];

/**
 * Linker script symbol for the end address of the global flash text
 * region
 */
extern const uint32_t __etext[];

/**
 * Linker script symbol for the start address of the global RAM
 * text region
 */
extern const uint32_t __ram_text_start[];

/**
 * Linker script symbol for the end address of the global RAM text
 * region
 */
extern const uint32_t __ram_text_end[];

/**
 * Linker script symbol for the start address of the secret flash
 * text region
 */
extern const uint32_t __secret_flash_text_start[];

/**
 * Linker script symbol for the end address of the secret flash text
 * region
 */
extern const uint32_t __secret_flash_text_end[];

/**
 * Linker script symbol for the start address of the secret RAM
 * text region
 */
extern const uint32_t __secret_ram_text_start[];

/**
 * Linker script symbol for the end address of the secret RAM text
 * region
 */
extern const uint32_t __secret_ram_text_end[];

/**
 * Linker script symbol for the start address of the global background region
 */
extern const uint32_t __background_data_region_start[];

/**
 * Linker script symbol for the end address of static RAM (address of the next
 * byte after the end of  static RAM)
 */
extern const uint32_t __sram_end[];

#endif /* SOURCES_BUILDING_BLOCKS_CORTEX_M_STARTUP_H_ */
