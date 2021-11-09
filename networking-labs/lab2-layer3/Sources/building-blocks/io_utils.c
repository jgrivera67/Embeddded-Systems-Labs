/**
 * @file io_utils.c
 *
 * I/O utils implementation
 *
 * @author: German Rivera
 */
#include "io_utils.h"
#include "microcontroller.h"
#include "runtime_checks.h"

uint32_t read_32bit_mmio_register(const volatile uint32_t *io_reg_p)
{
    D_ASSERT(VALID_MMIO_ADDRESS(io_reg_p));
    return *io_reg_p;
}


void write_32bit_mmio_register(volatile uint32_t *io_reg_p, uint32_t value)
{
    D_ASSERT(VALID_MMIO_ADDRESS(io_reg_p));
    *io_reg_p = value;
}


uint16_t read_16bit_mmio_register(const volatile uint16_t *io_reg_p)
{
    D_ASSERT(VALID_MMIO_ADDRESS(io_reg_p));
    return *io_reg_p;
}


void write_16bit_mmio_register(volatile uint16_t *io_reg_p, uint16_t value)
{
    D_ASSERT(VALID_MMIO_ADDRESS(io_reg_p));
    *io_reg_p = value;
}


uint8_t read_8bit_mmio_register(const volatile uint8_t *io_reg_p)
{
    D_ASSERT(VALID_MMIO_ADDRESS(io_reg_p));
    return *io_reg_p;
}


void write_8bit_mmio_register(volatile uint8_t *io_reg_p, uint8_t value)
{
    D_ASSERT(VALID_MMIO_ADDRESS(io_reg_p));
    *io_reg_p = value;
}
