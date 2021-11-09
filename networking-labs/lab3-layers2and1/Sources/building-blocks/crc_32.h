/**
 * @file crc_32.h
 *
 * Hardware-based CRC-32 driver interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_CRC_32_H_
#define SOURCES_BUILDING_BLOCKS_CRC_32_H_

#include <stdint.h>
#include <stddef.h>

void crc_32_accelerator_init(void);

uint32_t crc_32_accelerator_run(const void *data_buf_p, size_t num_bytes);

#endif /* SOURCES_BUILDING_BLOCKS_CRC_32_H_ */
