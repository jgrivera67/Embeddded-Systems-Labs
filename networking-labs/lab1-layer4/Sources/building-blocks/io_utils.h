/**
 * @file io_utils.h
 *
 * I/O utils interface
 *
 * @author: German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_IO_UTILS_H_
#define SOURCES_BUILDING_BLOCKS_IO_UTILS_H_

#include <stdint.h>
#include "runtime_checks.h"

/*
 * Bit field accessor macros
 */

#define BIT(_bit_index)     (UINT32_C(0x1) << (_bit_index))

#define MULTI_BIT_MASK(_most_significant_bit_index,                     \
                       _least_significant_bit_index)                    \
        (BIT(_most_significant_bit_index) |                             \
         ((BIT(_most_significant_bit_index) - 1) &                      \
          (UINT32_MAX << (_least_significant_bit_index))))

#define GET_BIT_FIELD(_container, _bit_mask, _bit_shift) \
    (((_container) & (_bit_mask)) >> (_bit_shift))

#define CLEAR_BIT_FIELD(_container, _bit_mask) \
    do {                                                                \
        (_container) &= ~(_bit_mask);                                   \
    } while (0)

#define SET_BIT_FIELD(_container, _bit_mask, _bit_shift, _value) \
    do {                                                                    \
        (_container) &= ~(_bit_mask);                                       \
        if ((_value) != 0) {                                                \
            D_ASSERT(((uint32_t)(_value) << (_bit_shift)) <= (_bit_mask));  \
            (_container) |=                                                 \
                ((uint32_t)(_value) << (_bit_shift)) & (_bit_mask);         \
        }                                                                   \
    } while (0)

/*
 * Stores a value for a given bit field in _Container, using
 * the ARMv8 BFI instruction
 */
#define SET_BIT_FIELD2(_container, _bit_shift, _bit_width, _value) \
  do {                                                                  \
    asm volatile (                                                      \
      "bfi %[container], %[value], #" STRINGIFY_CONSTANT(_bit_shift)    \
      ", #" STRINGIFY_CONSTANT(_bit_width) "\n\t"                       \
      : [container] "+r" (_container)                       			\
      : [value] "r" (_value)                                            \
    );                                                                  \
  } while (0)

/*
 * Extracts the value for a given bit field in _container, using the
 * UBFX ARMv8 instruction
 */
#define GET_BIT_FIELD2(_container, _bit_shift, _bit_width, _value) \
  do {                                                                  \
    asm volatile (                                                      \
      "ubfx %[value], %[container], #" STRINGIFY_CONSTANT(_bit_shift)   \
      ", #" STRINGIFY_CONSTANT(_bit_width) "\n\t"                       \
      : [value] "=r" (_value)                                           \
      : [container] "r" (_container)                                    \
    );                                                                  \
  } while (0)

/*
 * Build a literal string from a numeric constant
 */
#define STRINGIFY_CONSTANT(_constant) \
        __STRINGIFY_EXPANDED_CONSTANT(_constant)

#define __STRINGIFY_EXPANDED_CONSTANT(_expanded_constant) \
        #_expanded_constant

#if __STDC_VERSION__ == 201112L

/**
 * Generic macro to read an MMIO register of any width (8, 16 or 32 bits)
 *
 * @param _io_reg_p pointer to a memory-mapped I/O register
 *
 * @return value read
 */
#define READ_MMIO_REGISTER(_io_reg_p) \
        _Generic((_io_reg_p), \
                 volatile uint32_t *: read_32bit_mmio_register, \
                 volatile const uint32_t *: read_32bit_mmio_register, \
                 volatile uint16_t *: read_16bit_mmio_register, \
                 volatile const uint16_t *: read_16bit_mmio_register, \
                 volatile uint8_t *: read_8bit_mmio_register, \
                 volatile const uint8_t *: read_8bit_mmio_register, \
                 default: -1)(_io_reg_p)

/**
 * Generic macro to write an MMIO register of any width (8, 16 or 32 bits)
 *
 * @param _io_reg_p pointer to a memory-mapped I/O register
 * @param _value value to write
 */
#define WRITE_MMIO_REGISTER(_io_reg_p, _value) \
        _Generic((_io_reg_p), \
                 volatile uint32_t *: write_32bit_mmio_register, \
                 volatile uint16_t *: write_16bit_mmio_register, \
                 volatile uint8_t *: write_8bit_mmio_register, \
                 default: -1)(_io_reg_p, _value)

#else

#define READ_MMIO_REGISTER(_io_reg_p)  			(*(_io_reg_p))

#define WRITE_MMIO_REGISTER(_io_reg_p, _value)  (*(_io_reg_p) = (_value))

#endif /* __STDC_VERSION__ == 201112L */

uint32_t read_32bit_mmio_register(const volatile uint32_t *io_reg_p);

void write_32bit_mmio_register(volatile uint32_t *io_reg_p, uint32_t value);

uint16_t read_16bit_mmio_register(const volatile uint16_t *io_reg_p);

void write_16bit_mmio_register(volatile uint16_t *io_reg_p, uint16_t value);

uint8_t read_8bit_mmio_register(const volatile uint8_t *io_reg_p);

void write_8bit_mmio_register(volatile uint8_t *io_reg_p, uint8_t value);

#endif /* SOURCES_BUILDING_BLOCKS_IO_UTILS_H_ */
