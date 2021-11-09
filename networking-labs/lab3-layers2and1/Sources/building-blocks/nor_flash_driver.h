/**
 * @file nor_flash_driver.h
 *
 * NOR flash driver interface
 *
 * @author: German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_NOR_FLASH_DRIVER_H_
#define SOURCES_BUILDING_BLOCKS_NOR_FLASH_DRIVER_H_

#include "runtime_checks.h"
#include "microcontroller.h"
#include <stddef.h>

/**
 * Sector size (in bytes) of the MCU's program flash memory
 */
#if defined(KL25Z_MCU)
#define NOR_FLASH_SECTOR_SIZE    1024
#elif defined(K64F_MCU)
#define NOR_FLASH_SECTOR_SIZE    (4 * 1024)
#else
#error "No Microcontroller defined"
#endif

/**
 * Address of last NOR flash sector
 */
#define NOR_FLASH_LAST_SECTOR_ADDR \
		(MCU_FLASH_BASE_ADDR + MCU_FLASH_SIZE - NOR_FLASH_SECTOR_SIZE)

/**
 * Address of application configuration data in NOR flash
 */
#define NOR_FLASH_APP_CONFIG_ADDR	NOR_FLASH_LAST_SECTOR_ADDR


void nor_flash_init(void);

error_t nor_flash_write(uintptr_t dest_addr,
                        const void *src_addr,
                        size_t src_size);

#endif /* SOURCES_BUILDING_BLOCKS_NOR_FLASH_DRIVER_H_ */
