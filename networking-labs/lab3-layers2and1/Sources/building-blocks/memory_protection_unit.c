/**
 * @file memory_protection_unit.c
 *
 * Memory protection unit services implementation
 *
 * @author: German Rivera
 */
#include "memory_protection_unit.h"
#include "runtime_checks.h"
#include "io_utils.h"
#include <MK64F12.h>
#include <stdbool.h>

/**
 * Const fields of the MPU device
 */
struct mpu_device {
#   define MPU_DEVICE_SIGNATURE  GEN_SIGNATURE('M', 'P', 'U', ' ')
    uint32_t signature;
    MPU_Type *mmio_regs_p;
    struct mpu_device_var *var_p;
};

/**
 * Non-const fields of the MPU device
 */
struct mpu_device_var {
    bool initialized;
    uint8_t num_regions;
    uint8_t num_defined_global_regions;
};

static struct mpu_device_var g_mpu_var = {
    .initialized = false,
    .num_regions = 0,
    .num_defined_global_regions = 0,
};

static const struct mpu_device g_mpu = {
    .signature = MPU_DEVICE_SIGNATURE,
    .mmio_regs_p = (MPU_Type *)MPU_BASE,
    .var_p = &g_mpu_var,
};


/*
 * Disable MPU
 */
void mpu_disable(void)
{
    D_ASSERT(g_mpu.signature == MPU_DEVICE_SIGNATURE);

    MPU_Type *const mpu_regs_p = g_mpu.mmio_regs_p;

#   ifdef USE_MPU
    struct mpu_device_var *const mpu_var_p = g_mpu.var_p;

    D_ASSERT(mpu_var_p->initialized);
#	endif

    write_32bit_mmio_register(&mpu_regs_p->CESR, 0);
}

