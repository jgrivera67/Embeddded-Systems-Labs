/**
 * @file stack_trace.h
 *
 * Stack trace service interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_STACK_TRACE_H_
#define SOURCES_BUILDING_BLOCKS_STACK_TRACE_H_

#include <stdint.h>

void stack_trace_capture(uint_fast8_t num_entries_to_skip,
	                     uintptr_t trace_buff[],
	                     uint_fast8_t *num_entries_p);

#endif /* SOURCES_BUILDING_BLOCKS_STACK_TRACE_H_ */
