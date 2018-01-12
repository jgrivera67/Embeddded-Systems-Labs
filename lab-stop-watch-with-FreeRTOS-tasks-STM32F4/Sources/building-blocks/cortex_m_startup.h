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
 * Linker script symbol for the start address of the initialization values for global variables
 * in Flash
 */
extern const uint32_t _sidata[];

/**
 * Linker script symbol for the start address of the initialized global variables
 * section in RAM
 */
extern uint32_t __data_start__[];

/**
 * Linker script symbol for the end address of the initialized global variables
 * section in RAM
 */
extern uint32_t __data_end__[];

/**
 * Linker script symbol for the start address of the non-initialized global variables
 * section in RAM
 */
extern uint32_t __bss_start__[];

/**
 * Linker script symbol for the end address of the non-initialized global variables
 * section in RAM
 */
extern uint32_t __bss_end__[];

/**
 * Linker script symbol for the start address of the main stack in RAM
 */
extern uint32_t __stack_start__[];

/**
 * Linker script symbol for the end address of the main stack in RAM
 */
extern uint32_t __stack_end__[];

/**
 * Linker script symbol for the start address of the global flash
 * text region
 */
extern const uint32_t __flash_text_start__[];

/**
 * Linker script symbol for the end address of the global flash text
 * region
 */
extern const uint32_t __flash_text_end__[];

/**
 * Linker script symbol for the start address of the flash constants range
 */
extern const uint32_t __flash_constants_start__[];

/**
 * Linker script symbol for the end address of the flash constants range
 */
extern const uint32_t __flash_constants_end__[];

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
