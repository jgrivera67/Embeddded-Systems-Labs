/**
 * @file runtime_checks.h
 *
 * Runtime checks services interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_RUNTIME_CHECKS_H_
#define SOURCES_BUILDING_BLOCKS_RUNTIME_CHECKS_H_

#include <stdint.h>
#include <stdbool.h>
#include "microcontroller.h"

/*
 * Macros to give a hint to the compiler for static branch prediction,
 * for infrequently executed code paths, such as error cases.
 */
#define _INFREQUENTLY_TRUE_(_condition)    __builtin_expect((_condition), false)
#define _INFREQUENTLY_FALSE_(_condition)    __builtin_expect((_condition), true)

#ifndef NDEBUG
/**
 * Macro to check debug-only assertions
 */
#define D_ASSERT(_cond) \
        do {                                                                 \
            if (_INFREQUENTLY_TRUE_(!(_cond))) {                             \
                debug_assert_failure(#_cond, __func__, __FILE__,             \
                                     __LINE__);                                \
            }                                                                \
        } while (0)

void debug_assert_failure(const char *cond_str, const char *func_name,
                          const char *file_name, int line);

#else

#define D_ASSERT(_cond)

#endif

/**
 * Generate a signature for an object type.
 * _a, _b, _c, _d must be printable ASCII characters.
 */
#define GEN_SIGNATURE(_a, _b, _c, _d) \
        (((uint32_t)(_d) << 24) | \
         ((uint32_t)(_c) << 16) | \
         ((uint32_t)(_b) << 8)  | \
         (uint32_t)(_a))

/**
 * Tell if the value of _error corresponds to an error
 */
#define IS_ERROR(_error)    _INFREQUENTLY_TRUE_((_error) != 0)

/**
 * Convenience macro to call capture_error()
 */
#define CAPTURE_ERROR(_error_description, _arg1, _arg2) \
        capture_error(_error_description, \
                      (uintptr_t)_arg1, \
                      (uintptr_t)_arg2)

/**
 * Check that an mmio address is in the valid MMIO space
 */
#define VALID_MMIO_ADDRESS(_io_addr) \
        (((uintptr_t)(_io_addr) >= MCU_PERIPHERAL_BRIDGE_MIN_ADDR &&    \
          (uintptr_t)(_io_addr) <= MCU_PERIPHERAL_BRIDGE_MAX_ADDR) ||   \
         ((uintptr_t)(_io_addr) >= MCU_PRIVATE_PERIPHERALS_MIN_ADDR &&  \
          (uintptr_t)(_io_addr) <= MCU_PRIVATE_PERIPHERALS_MAX_ADDR) || \
         ((uintptr_t)(_io_addr) >= MCU_MTB_MIN_ADDR &&                  \
          (uintptr_t)(_io_addr) <= MCU_MTB_MAX_ADDR))

/**
 * Check that an address is in flash memory and it is not address 0x0
 */
#define VALID_FLASH_ADDRESS(_addr) \
        ((uintptr_t)(_addr) > MCU_FLASH_BASE_ADDR &&                         \
         (uintptr_t)(_addr) < MCU_FLASH_BASE_ADDR + MCU_FLASH_SIZE)

/**
 * Check that an address is in RAM memory
 */
#define VALID_RAM_ADDRESS(_addr) \
        ((uintptr_t)(_addr) >= MCU_SRAM_BASE_ADDR &&                        \
         (uintptr_t)(_addr) < MCU_SRAM_BASE_ADDR + MCU_SRAM_SIZE)

/**
 * Check that a RAM pointer is valid
 */
#define VALID_RAM_POINTER(_data_ptr, _alignment) \
        (VALID_RAM_ADDRESS(_data_ptr) &&                                    \
         (uintptr_t)(_data_ptr) % (_alignment) == 0)

/**
 * Stack overflow marker for an execution stack
 */
#define STACK_OVERFLOW_MARKER   GEN_SIGNATURE('S', 'T', 'K', 'O')

/**
 * Stack underflow marker for an execution stack
 */
#define STACK_UNDERFLOW_MARKER   GEN_SIGNATURE('S', 'T', 'K', 'U')

/**
 * Runtime error code type. It encodes the code location where
 * the error originated.
 */
typedef uintptr_t error_t;


error_t capture_error(const char *error_description,
                      uintptr_t arg1,
                      uintptr_t arg2);

void fatal_error_handler(error_t error);

#endif /* SOURCES_BUILDING_BLOCKS_RUNTIME_CHECKS_H_ */
