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
#include <stdbool.h>

struct rtos_task;

bool find_previous_stack_frame(
    const void *start_pc,
    const uint32_t *stack_bottom_end_p,
    const uint32_t **frame_pointer_p,
    uintptr_t *prev_return_address_p);

void stack_trace_capture(uint_fast8_t num_entries_to_skip,
						 const void *start_pc,
						 const void *start_frame_pointer,
						 const struct rtos_task *task_p,
	                     uintptr_t trace_buff[],
	                     uint_fast8_t *num_entries_p);

#endif /* SOURCES_BUILDING_BLOCKS_STACK_TRACE_H_ */
