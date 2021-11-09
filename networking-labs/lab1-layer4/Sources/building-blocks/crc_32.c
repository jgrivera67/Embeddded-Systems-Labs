/**
 * @file crc_32.h
 *
 * Hardware-based CRC-32 driver implementation
 *
 * @author German Rivera
 */
#include "crc_32.h"
#include "runtime_checks.h"
#include "io_utils.h"

/**
 * Const fields of the CRC device (to be placed in flash)
 */
struct crc_device {
#   define CRC_DEVICE_SIGNATURE  GEN_SIGNATURE('C', 'R', 'C', ' ')
    uint32_t signature;
    CRC_Type *mmio_regs_p;
    struct crc_device_var *var_p;
};

/**
 * Non-const fields of an CRC device (to be placed in SRAM)
 */
struct crc_device_var {
    bool initialized;
};

static struct crc_device_var g_crc_var = {
    .initialized = false,
};

static const struct crc_device g_crc = {
    .signature = CRC_DEVICE_SIGNATURE,
    .mmio_regs_p = (CRC_Type *)CRC_BASE,
    .var_p = &g_crc_var,
};


/**
 * Initializes CRC hardware module
 */
void crc_32_accelerator_init(void)
{
    uint32_t reg_value;

    D_ASSERT(g_crc.signature == CRC_DEVICE_SIGNATURE);
    struct crc_device_var *const crc_var_p = g_crc.var_p;

    D_ASSERT(!crc_var_p->initialized);

    CRC_Type *const crc_regs_p = g_crc.mmio_regs_p;

    /*
     * Enable the Clock to the CRC Module
     */
    reg_value = READ_MMIO_REGISTER(&SIM_SCGC6);
    reg_value |= SIM_SCGC6_CRC_MASK;
    WRITE_MMIO_REGISTER(&SIM_SCGC6, reg_value);

    WRITE_MMIO_REGISTER(&crc_regs_p->CTRL, 0);
    crc_var_p->initialized = true;
}


/**
 * Calculate CRC-32 using the CRC hardware module
 *
 * @param data_buf_p	Pointer to data buffer for which
 *                      CRC is to be computed
 * @param num_bytes		Size of the data buffer
 *
 * @return Computed CRC-32
 */
uint32_t crc_32_accelerator_run(const void *data_buf_p, size_t num_bytes)
{
#   define CRC_32_POLYNOMIAL UINT32_C(0x04C11DB7)

    uint32_t reg_value;
    struct crc_device_var *const crc_var_p = g_crc.var_p;

#   ifdef USE_MPU
    bool privileged_caller = rtos_enter_privileged_mode();
#   endif

    D_ASSERT(crc_var_p->initialized);

    CRC_Type *const crc_regs_p = g_crc.mmio_regs_p;

    /*
     * Configure CRC functionality:
     * - Select 32-bit CRC
     * - Don't do 1's complement of input data
     * - Bits in bytes are transposed but bytes are not transposed, when writing
     *   to DATA register.
     * - Both bits in bytes and bytes are transposed when reading DATA register
     */
    reg_value = READ_MMIO_REGISTER(&crc_regs_p->CTRL);
    reg_value |= CRC_CTRL_TCRC_MASK;
    reg_value &= ~CRC_CTRL_FXOR_MASK;
    SET_BIT_FIELD(reg_value, CRC_CTRL_TOT_MASK, CRC_CTRL_TOT_SHIFT, 0x1);
    SET_BIT_FIELD(reg_value, CRC_CTRL_TOTR_MASK, CRC_CTRL_TOTR_SHIFT, 0x2);
    WRITE_MMIO_REGISTER(&crc_regs_p->CTRL, reg_value);

    /*
     * Program CRC-32 polynomial
     */
    write_32bit_mmio_register(&crc_regs_p->GPOLY, CRC_32_POLYNOMIAL);

    /*
     * Program seed
     */
    reg_value = READ_MMIO_REGISTER(&crc_regs_p->CTRL);
    reg_value |= CRC_CTRL_WAS_MASK;
    WRITE_MMIO_REGISTER(&crc_regs_p->CTRL, reg_value);
    WRITE_MMIO_REGISTER(&crc_regs_p->DATA, UINT32_MAX);
    reg_value &= ~CRC_CTRL_WAS_MASK;
    WRITE_MMIO_REGISTER(&crc_regs_p->CTRL, reg_value);

    /*
     * Write data values (most significant byte first - Big endian):
     */
    size_t num_words = num_bytes / 4;
    size_t remaining_bytes = num_bytes % 4;
    const uint8_t *p = data_buf_p;
    const uint8_t *end_p = p + num_words * 4;

    for ( ; p != end_p; p += 4) {
        reg_value = (((uint32_t)p[0] << 24) |
                     ((uint32_t)p[1] << 16) |
                     ((uint32_t)p[2] << 8) |
                     (uint32_t)p[3]);
        WRITE_MMIO_REGISTER(&crc_regs_p->DATA, reg_value);
    }

    switch (remaining_bytes) {
    case 3:
        WRITE_MMIO_REGISTER(&crc_regs_p->ACCESS8BIT.DATAHU, p[0]);
        WRITE_MMIO_REGISTER(&crc_regs_p->ACCESS8BIT.DATAHL, p[1]);
        WRITE_MMIO_REGISTER(&crc_regs_p->ACCESS8BIT.DATALU, p[2]);
        break;
    case 2:
        WRITE_MMIO_REGISTER(&crc_regs_p->ACCESS8BIT.DATAHU, p[0]);
        WRITE_MMIO_REGISTER(&crc_regs_p->ACCESS8BIT.DATAHL, p[1]);
        break;
    case 1:
        WRITE_MMIO_REGISTER(&crc_regs_p->ACCESS8BIT.DATAHU, p[0]);
        break;
    }

    /*
     * Retrieve calculated CRC:
     */
    reg_value = READ_MMIO_REGISTER(&crc_regs_p->DATA);

#   ifdef USE_MPU
    if (!privileged_caller) {
        rtos_exit_privileged_mode();
    }
#   endif

    return reg_value;
}

