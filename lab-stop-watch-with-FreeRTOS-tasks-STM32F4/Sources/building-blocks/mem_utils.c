/**
 * @file mem_utils.c
 *
 * Memory utilities implementation
 *
 * @author: German Rivera
 */

#include "mem_utils.h"
#include "runtime_checks.h"
#include <stdint.h>

#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

#define CRC_32_POLYNOMIAL UINT32_C(0x04c11db7)

/**
 * Computes the CRC-32 checksum for a given block of memory
 *
 * @param start_addr: start address of the memory block
 * @param size: size in bytes
 *
 * @return calculated CRC value
 */
uint32_t mem_checksum(const void *start_addr, uint32_t size)
{
    uint32_t crc = UINT32_C(0xffffffff);
    const uint8_t *end_p = (uint8_t *)start_addr + size;

    for (const uint8_t *byte_p = start_addr; byte_p < end_p; byte_p ++) {
        uint8_t c = *byte_p;

        for (uint32_t i = 0; i < 8; i ++) {
            if ((c ^ crc) & 1) {
                crc >>= 1;
                c >>= 1;
                crc ^= CRC_32_POLYNOMIAL;
            } else {
                crc >>= 1;
                c >>= 1;
            }
        }
    }

    return crc;
}


/**
 * Copies a 32-bit aligned block of memory from one location to another
 *
 * @param src: pointer to source location
 * @param dst: pointer to destination location
 * @param size: size in bytes
 *
 * @pre dst must be a 4-byte aligned address
 * @pre src must be a 4-byte aligned address
 * @pre size must be a multiple of 4
 */
void memcpy32(uint32_t *dst, const uint32_t *src, uint32_t size)
{
	register uint32_t *dst_cursor = dst;
	register const uint32_t *src_cursor = src;

	D_ASSERT((uintptr_t)dst % sizeof(uint32_t) == 0);
	D_ASSERT((uintptr_t)src % sizeof(uint32_t) == 0);
	D_ASSERT(size % sizeof(uint32_t) == 0);

	uint32_t num_words = size / sizeof(uint32_t);
	register uint32_t *dst_end = dst + ((num_words / 4) * 4);

	while (dst_cursor != dst_end) {
#if  1
		dst_cursor[0] = src_cursor[0];
		dst_cursor[1] = src_cursor[1];
		dst_cursor[2] = src_cursor[2];
		dst_cursor[3] = src_cursor[3];
		dst_cursor += 4;
		src_cursor += 4;
#else
		*dst_cursor++ = *src_cursor++;
		*dst_cursor++ = *src_cursor++;
		*dst_cursor++ = *src_cursor++;
		*dst_cursor++ = *src_cursor++;
#endif
	}

	switch (num_words % 4) {
	case 3:
		*dst_cursor++ = *src_cursor++;
	case 2:
		*dst_cursor++ = *src_cursor++;
	case 1:
		*dst_cursor++ = *src_cursor++;
	}
}

/**
 * Initializes all bytes of a 32-bit-aligned memory block to a given value
 *
 * @param dst: pointer to destination location
 * @param size: size in bytes
 *
 * @pre dst must be a 4-byte aligned address
 * @pre size must be a multiple of 4
 */
void memset32(uint32_t *dst, uint_fast8_t byte_value, uint32_t size)
{
	register uint32_t *dst_cursor = dst;
	register uint32_t word_value;

	D_ASSERT((uintptr_t)dst % sizeof(uint32_t) == 0);
	D_ASSERT(size % sizeof(uint32_t) == 0);

	uint32_t num_words = size / sizeof(uint32_t);
	register uint32_t *dst_end = dst + ((num_words / 4) * 4);

	if (byte_value != 0) {
		word_value = (((uint32_t)byte_value << 24) |
					  ((uint32_t)byte_value << 16) |
					  ((uint32_t)byte_value << 8) |
					  byte_value);
	} else {
		word_value = 0;
	}

	while (dst_cursor != dst_end) {
#if 1
		dst_cursor[0] = word_value;
		dst_cursor[1] = word_value;
		dst_cursor[2] = word_value;
		dst_cursor[3] = word_value;
		dst_cursor += 4;
#else
		*dst_cursor++ = word_value;
		*dst_cursor++ = word_value;
		*dst_cursor++ = word_value;
		*dst_cursor++ = word_value;
#endif
	}

	switch (num_words % 4) {
	case 3:
		*dst_cursor++ = word_value;
	case 2:
		*dst_cursor++ = word_value;
	case 1:
		*dst_cursor++ = word_value;
	}
}

