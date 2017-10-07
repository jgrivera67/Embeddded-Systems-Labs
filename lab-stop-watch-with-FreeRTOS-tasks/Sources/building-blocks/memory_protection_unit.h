/**
 * @file memory_protection_unit.h
 *
 * Memory protection unit services interface
 *
 * @author: German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_MEMORY_PROTECTION_UNIT_H_
#define SOURCES_BUILDING_BLOCKS_MEMORY_PROTECTION_UNIT_H_

#include <stdint.h>
#include <stdbool.h>
#include "microcontroller.h"

/**
 * MPU regions assignment
 */
enum mpu_region_id {
	GLOBAL_BACKGROUND_DATA_REGION = 0,
	GLOBAL_FLASH_CODE_REGION,
    GLOBAL_RAM_CODE_REGION,
    GLOBAL_INTERRUPT_STACK_REGION,
    THREAD_STACK_DATA_REGION,
    PRIVATE_DATA_REGION,
    PRIVATE_CODE_REGION,
	/* New regions need be added above this line */
	NUM_MPU_REGIONS
};

/**
 * Saved MPU region descriptor
 */
struct mpu_region_descriptor {
	volatile uint32_t rbar_value;
	volatile uint32_t rasr_value;
};

/**
 * Thread-private MPU regions
 *
 * @field Stack_Region MPU region for the thread's stack
 * @field Object_Data_Region MPU region for current provate object data
 *        region for the thread.
 * @field Code_Region MPU region for the current private code region
 *        for the thread.
 * @field Writable_Background_Region_Enabled Flag indicating if the
 *        background region is currently writable for the thread (true) or
 *        read-only (false).
 */
struct thread_regions {
	struct mpu_region_descriptor stack_region;
	struct mpu_region_descriptor private_data_region;
	struct mpu_region_descriptor private_code_region;
	bool writable_background_region_enabled;
};

enum data_region_permissions {
	PERM_NONE = 0,
	READ_ONLY,
	READ_WRITE
};

void mpu_init(void);

void mpu_disable(void);

void mpu_enable(void);

/**
 * Restore thread-private MPU regions
 *
 * NOTE: This function is to be invoked only from the RTOS
 * context switch code and with the background region enabled
 */
void restore_thread_mpu_regions(const struct thread_regions *thread_regions_p);

/**
 * Save thread-private MPU regions
 *
 * NOTE: This function is to be invoked only from the RTOS
 * context switch code and with the background region enabled
 */
void save_thread_mpu_regions(struct thread_regions *thread_regions_p);

void set_cpu_writable_background_region(bool enabled);

bool set_cpu_writable_background_region(bool enabled);

bool is_mpu_enabled(void);

void restore_private_code_region(struct mpu_region_descriptor saved_region);

void restore_private_data_region(struct mpu_region_descriptor saved_region);

void set_private_code_region_no_save(void *first_addr, void *last_addr);

void set_private_code_region(void *first_addr, void *last_addr,
		                     struct mpu_region_descriptor *old_region_p);

void set_private_data_region_no_save(void *start_addr, size_t size,
		                             enum data_region_permissions permissions);

void set_private_data_region(void *start_addr, size_t size,
		                     enum data_region_permissions permissions,
		                     struct mpu_region_descriptor *old_region_p);

#endif /* SOURCES_BUILDING_BLOCKS_MEMORY_PROTECTION_UNIT_H_ */
