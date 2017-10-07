/**
 * @file system_clocks.h
 *
 * System clocks initialization interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_SYSTEM_CLOCKS_H_
#define SOURCES_BUILDING_BLOCKS_SYSTEM_CLOCKS_H_

#include <stdint.h>

void system_clocks_init(void);

uint32_t get_pll_frequency_in_hz(void);

#endif /* SOURCES_BUILDING_BLOCKS_SYSTEM_CLOCKS_H_ */
