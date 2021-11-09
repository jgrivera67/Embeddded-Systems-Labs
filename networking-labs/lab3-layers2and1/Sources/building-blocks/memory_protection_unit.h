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

/**
 * MPU data region range
 *
 * NOTE: If MPU_READ_ONLY_REGION is set in flags, only read access is allowed.
 * Otherwise, read/write access is allowed.
 */
struct mpu_region_range {
    /**
     * Address of the first byte of the region
     */
    const void *start_addr;

    /**
     * Address of the first byte after the region
     */
    const void *end_addr;

    /**
     * Access flags for the region
     */
    uint32_t flags;
#   define  MPU_REGION_INACTIVE     UINT32_C(0x1)
#   define  MPU_REGION_READ_ONLY    UINT32_C(0x2)
};


void mpu_disable(void);

#endif /* SOURCES_BUILDING_BLOCKS_MEMORY_PROTECTION_UNIT_H_ */
