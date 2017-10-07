/**
 * @file cpu_reset_counter.h
 *
 * CPU reset counter interface
 *
 * @author: German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_CPU_RESET_COUNTER_H_
#define SOURCES_BUILDING_BLOCKS_CPU_RESET_COUNTER_H_

#include <stdbool.h>
#include <stdint.h>

bool valid_cpu_reset_counter(void);

void initialize_cpu_reset_counter(void);

void increment_cpu_reset_counter(void);

uint32_t read_cpu_reset_counter(void);

#endif /* SOURCES_BUILDING_BLOCKS_CPU_RESET_COUNTER_H_ */
