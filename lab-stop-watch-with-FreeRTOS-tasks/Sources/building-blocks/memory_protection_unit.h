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
#include <stdlib.h>
#include "microcontroller.h"

#if __MPU_PRESENT == 1
#define REGION_ALIGNMENT(_type)		ROUND_UP_TO_POWER_OF_2(sizeof(_type))

#define ROUND_UP_TO_POWER_OF_2(_x) \
           ((_x) <= 32 ? 32 :                              \
            ((_x) <= 64 ? 64 :                             \
             ((_x) <= 128 ? 128 :                          \
              ((_x) <= 256 ? 256 :                         \
               ((_x) <= 512 ? 512 :                        \
                ((_x) <= 1024 ? 1024 :                     \
                 ((_x) <= (2*1024) ? (2*1024) :            \
                  ((_x) <= (4*1024) ? (4*1024) :           \
                   ((_x) <= (8*1024) ? (8*1024) :          \
                    ((_x) <= (16*1024) ? (16*1024) :  	   \
                     ((_x) <= (32*1024) ? (32*1024) :      \
                      ((_x) <= (64*1024) ? (64*1024) :     \
                       ((_x) <= (128*1024) ? (128*1024) :  \
                        ((_x) <= (256*1024) ? (256*1024) : \
                         (512*1024)))))))))))))))

#else
#define REGION_ALIGNMENT(_type)		MPU_REGION_ALIGNMENT
#endif

/**
 * MPU regions assignment
 */
enum mpu_region_id {
    GLOBAL_BACKGROUND_DATA_REGION = 0,
    GLOBAL_FLASH_CODE_REGION = 1,
    GLOBAL_RAM_CODE_REGION = 2,
    GLOBAL_INTERRUPT_STACK_REGION = 3,
    GLOBAL_MPU_IO_REGION = 4,
    THREAD_STACK_DATA_REGION = 5,
    PRIVATE_DATA_REGION = 6,
    PRIVATE_CODE_REGION = 7,

    /* If additional regions are necessary, they must be defined above */
    MAX_NUM_MPU_REGIONS
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

bool mpu_disable(void);

void mpu_enable(void);

/**
 * Initializes the thread-private MPU region descriptors
 *
 * NOTE: This function is to be invoked only from the RTOS
 * code that initializes the context of a thread before it is
 * switched in. The first time that the thread is context-switch-in,
 * the corresponding actual MPU region descriptors will be loaded with
 * the values set here.
 */
void initialize_thread_mpu_regions(void *stack_base_addr,
		                           size_t stack_size,
								   struct thread_regions *thread_regions_p);

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

bool set_writable_background_region(bool enabled);

bool is_mpu_enabled(void);

void restore_private_code_region(const struct mpu_region_descriptor *saved_region_p);

void restore_private_data_region(const struct mpu_region_descriptor *saved_region_p);

void set_private_code_region_no_save(void *first_addr, void *last_addr);

void set_private_code_region(void *first_addr, void *last_addr,
		                     struct mpu_region_descriptor *old_region_p);

void set_private_data_region_no_save(void *start_addr, size_t size,
		                             enum data_region_permissions permissions);

void set_private_data_region(void *start_addr, size_t size,
		                     enum data_region_permissions permissions,
		                     struct mpu_region_descriptor *old_region_p);

#endif /* SOURCES_BUILDING_BLOCKS_MEMORY_PROTECTION_UNIT_H_ */
