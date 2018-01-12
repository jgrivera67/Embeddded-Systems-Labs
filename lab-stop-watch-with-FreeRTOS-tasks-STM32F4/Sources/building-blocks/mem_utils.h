/**
 * @file mem_utils.h
 *
 * Memory utilities interface
 *
 * @author: German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_MEM_UTILS_H_
#define SOURCES_BUILDING_BLOCKS_MEM_UTILS_H_

#include <stdint.h>
#include <stddef.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_array) \
        (sizeof(_array) / sizeof((_array)[0]))
#endif

#define HOW_MANY(_m, _n)    (((size_t)(_m) - 1) / (_n) + 1)

#define ROUND_UP(_m, _n)    (HOW_MANY(_m, _n) * (_n))

#define ROUND_DOWN(_m, _n)    (((_m) / (_n)) * (_n))

/**
 * Given pointer to a struct '_enclosed_struc_p' that is contained in
 * another struct 'enclosing_struct_type', as field '_enclosing_struct_field',
 * return a pointer to the enclosing struct.
 */
#define ENCLOSING_STRUCT(_enclosed_struct_p, _enclosing_struct_type, \
                         _enclosing_struct_field)                     \
    ((_enclosing_struct_type *)(                                     \
        (uintptr_t)(_enclosed_struct_p) -                              \
        offsetof(_enclosing_struct_type, _enclosing_struct_field)))

/*
 * Macro to tell the compiler to place a function in SRAM
 * instead of flash
 */
#define RAM_FUNC __attribute__ ((section (".ram_functions")))

uint32_t mem_checksum(const void *start_addr, uint32_t size);

void memcpy32(uint32_t *dst, const uint32_t *src, uint32_t size);

void memset32(uint32_t *dst, uint_fast8_t byte_value, uint32_t size);

#endif /* SOURCES_BUILDING_BLOCKS_MEM_UTILS_H_ */
