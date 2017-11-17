/**
 * @file stack_trace.c
 *
 * Stack trace service implementation
 *
 * @author German Rivera
 */
#include "stack_trace.h"
#include "microcontroller.h"
#include "io_utils.h"
#include "cortex_m_startup.h"
#include "rtos_wrapper.h"

static uint_fast8_t get_pushed_r7_stack_index(
                        thumb_instruction_t push_instruction)
{
    uint_fast8_t reg_list = (push_instruction & PUSH_OPERAND_REG_LIST_MASK);
    uint_fast8_t index = 0;

    /*
     * Check if registers r0 .. r6 are saved on the stack by the push
     * instruction
     */
    for (uint_fast8_t i = 0; i < 7; i ++) {
        if (reg_list & BIT(i)) {
            index ++;
        }
    }

    return index;
}


/**
 * Finds the previous stack frame while unwinding the current execution stack.
 *
 * NOTE: This function assumes that function prologs have the following code
 * pattern ([] means optional):
 *
 * push {[r4,] [r5,] [r6,] r7 [, lr]}   ([] means optional)
 * [sub sp, #imm7]
 * add r7, sp, #imm8
 *
 * @param[in] start_pc Current program counter
 * @param[in] stack_bottom_end_p Pointer to bottom end of the stack
 * @param[inout] frame_pointer_p Pointer to the frame pointer. On entry,
 *               *frame_pointer_p is the current frame pointer. On exit,
 *               *frame_pointer_p is the previous frame pointer in the call
 *               chain.
 * @param[out] prev_return_address_p Pointer to area where the previous return
 *             address in the call chain is to be stored.
 */
bool find_previous_stack_frame(
    const void *start_pc,
    const uint32_t *stack_bottom_end_p,
    const uint32_t **frame_pointer_p,
    uintptr_t *prev_return_address_p)
{
    thumb_instruction_t instruction;
    uint_fast8_t saved_r7_stack_index;
    const uint32_t *prev_frame_pointer;
    uintptr_t prev_return_address;
    const uint32_t *stack_pointer;
    const uint32_t *frame_pointer = *frame_pointer_p;
    uint_fast16_t stop_count = UINT16_MAX;

    const thumb_instruction_t *program_counter;

    if (start_pc == NULL) {
        uintptr_t return_address;

        CAPTURE_ARM_LR_REGISTER(return_address);
        D_ASSERT(!IS_CPU_EXCEPTION_RETURN((uintptr_t)return_address));
        program_counter = (thumb_instruction_t *)GET_CALL_ADDRESS(return_address);
    } else {
    	program_counter = start_pc;
        D_ASSERT(!IS_CPU_EXCEPTION_RETURN((uintptr_t)program_counter));

        /*
         * program_counter is supposed to be a code address with the THUM mode
         * flag already turned off.
         */
        D_ASSERT(((uintptr_t)program_counter & CPU_THUMB_CODE_FLAG) == 0);
    }

    if (!VALID_RAM_POINTER(frame_pointer, sizeof(uint32_t))) {
        return false;
    }

    /*
     * Scan instructions backwards looking for one of the 3 instructions in the
     * function prolog pattern:
     *
     * TODO: Add check to quit the loop if reached address is not in a code segment in flash or RAM
     */
    while (stop_count != 0) {
        instruction = *program_counter;
        if (IS_ADD_R7_SP_IMMEDITATE(instruction) ||
            IS_SUB_SP_IMMEDITATE(instruction) ||
            IS_PUSH_R7(instruction)) {
            break;
        }

        program_counter--;
        stop_count--;
    }

    if (stop_count == 0) {
        return false;
    }

    if (IS_ADD_R7_SP_IMMEDITATE(instruction)) {
        /*
         * The instruction to be executed is the 'add r7, ...' in the function
         * prolog.
         *
         * NOTE: the decoded operand of the add instruction is
         *    (instruction & ADD_SP_IMMEDITATE_OPERAND_MASK) << 2
         *
         * which is a byte offset that was added to the frame pointer. To
         * convert it to an stack entry index we need to divide it by 4,
         * so, the '/ 4' and the '<< 2' cancel each other.
         */
        stack_pointer = frame_pointer - (instruction & ADD_SP_IMMEDITATE_OPERAND_MASK);
        program_counter --;

        /*
         * Scan instructions backwards looking for the preceding 'sub sp, ...'
         * or 'push {...r7}':
         */
        while (stop_count != 0) {
            instruction = *program_counter;
            if (IS_SUB_SP_IMMEDITATE(instruction) ||
                IS_PUSH_R7(instruction)) {
                break;
            }

            program_counter--;
            stop_count--;
        }

        if (stop_count == 0) {
            return false;
        }
    } else {
        stack_pointer = frame_pointer;
    }

    if (IS_SUB_SP_IMMEDITATE(instruction)) {
        /*
         * The preceding instruction to be executed is the 'sub sp, ...' in the
         * function prolog.
         *
         * NOTE: the decoded operand of the sub instruction is
         *    (instruction & SUB_SP_IMMEDITATE_OPERAND_MASK) << 2
         *
         * which is a byte offset that was subtracted from the stack pointer. To
         * convert it to an stack entry index we need to divide it by 4,
         * so, the '/ 4' and the '<< 2' cancel each other.
         */
        stack_pointer += (instruction & SUB_SP_IMMEDITATE_OPERAND_MASK);
        program_counter --;

        /*
         * Scan instructions backwards looking for the preceding 'push {...r7}'
         */
        while (stop_count != 0) {
            instruction = *program_counter;
            if (IS_PUSH_R7(instruction)) {
                break;
            }

            program_counter--;
            stop_count--;
        }

        if (stop_count == 0) {
            return false;
        }
    }

    /*
     * The preceding instruction is the 'push {...r7...}'
     * at the beginning of the prolog:
     *
     * NOTE: At this point, stack_pointer has the value that SP had right
     * after executing the 'push {...r7...}'
     */
    if (!IS_PUSH_R7(instruction)) {
        return false;
    }

    saved_r7_stack_index = get_pushed_r7_stack_index(instruction);
    if (stack_pointer + saved_r7_stack_index >= stack_bottom_end_p) {
        return false;
    }

    prev_frame_pointer = (uint32_t *)stack_pointer[saved_r7_stack_index];
    if (!VALID_RAM_POINTER(prev_frame_pointer, sizeof(uint32_t))) {
        return false;
    }

    if (instruction & PUSH_OPERAND_INCLUDES_LR_MASK) {
        prev_return_address = stack_pointer[saved_r7_stack_index + 1];
        if ((prev_return_address & CPU_THUMB_CODE_FLAG) == 0) {
            return false;
        }
    } else {
        /*
         * Inline function with stack frame case
         */
        prev_return_address = (uintptr_t)(program_counter - 1);
    }

    *frame_pointer_p = prev_frame_pointer;
    *prev_return_address_p = prev_return_address;
    return true;
}


/*
 * Unwinds a given execution stack
 */
static void unwind_execution_stack(uint_fast8_t num_entries_to_skip,
                                   uintptr_t top_return_address,
                                   const uint32_t *frame_pointer,
                                   const uint32_t *stack_bottom_end_p,
                                   uintptr_t trace_buff[],
                                   uint8_t *num_entries_p)
{
    bool found_prev_stack_frame;
    uint_fast8_t max_num_entries = *num_entries_p;
    uintptr_t return_address;
    const thumb_instruction_t *program_counter;
    uint_fast8_t i = 0;

    /*
     * Since ARM Cortex-M cores run in thumb mode,
     * return addresses must have the lowest bit set:
     */
    if ((top_return_address & 0x1) == 0) {
        goto common_exit;
    }

    return_address = top_return_address;
    while (i < max_num_entries) {
        if (IS_CPU_EXCEPTION_RETURN(return_address)) {
            uintptr_t saved_pc;

            /*
             * Add stack-trace entry for the exception special
             * return address value:
             */
            trace_buff[i] = return_address;
            i ++;

            /*
             * We are using the MSP stack pointer, as we are being invoked
             * from an interrupt/exception handler
             */

            if (return_address == CPU_EXC_RETURN_TO_THREAD_MODE_USING_PSP ||
                return_address == CPU_EXC_RETURN_TO_THREAD_MODE_USING_PSP_FPU) {
                /*
                 * Interrupted code was using the PSP stack pointer
                 */
                const uint32_t *stack_pointer = (uint32_t *)(uintptr_t)__get_PSP();

                saved_pc = stack_pointer[6];
            } else {
                /*
                 * Interrupted code was using the MSP stack pointer. So,
                 * the interrupted code was another interrupt/exception handler.
                 * The frame pointer must be pointing to the top of the stack at
                 * the time when registers saving was done before branching to
                 * the interrupt/exception handler.
                 */
                saved_pc = frame_pointer[6]; //TODO: fix this
            }

            D_ASSERT((saved_pc & CPU_THUMB_CODE_FLAG) == 0);
            program_counter = (const thumb_instruction_t *)saved_pc;

            /*
             * Add stack-trace entry for the address where the interrupt/exception
             * happened:
             */
            trace_buff[i] = (uintptr_t)program_counter;
            i ++;
        } else {
            /*
             * The next stack-trace entry is the address of the instruction
             * preceding the instruction at the return address, unless we
             * have reached the bottom of the call chain.
             */
            thumb_instruction_t *instruction_p =
                (thumb_instruction_t*)GET_CODE_ADDRESS(return_address);

            if (IS_BLX(*(instruction_p - 1))) {
                instruction_p --;
            } else if (IS_BL32_FIRST_HALF(*(instruction_p - 2)) &&
                       IS_BL32_SECOND_HALF(*(instruction_p - 1))) {
                instruction_p -= 2;
            }

            if (num_entries_to_skip != 0) {
                num_entries_to_skip --;
            } else {
                trace_buff[i] = (uintptr_t)instruction_p;
                i ++;
            }

            program_counter = instruction_p - 1;
        }

        found_prev_stack_frame = find_previous_stack_frame(program_counter,
                                                           stack_bottom_end_p,
                                                           &frame_pointer,
                                                           &return_address);
        if (!found_prev_stack_frame) {
            break;
        }
    }

common_exit:
    *num_entries_p = i;
}


/**
 * Captures stack trace (call back-trace) for the caller
 *
 * @param num_entries_to_skip Number of stack-trace entries to skip at the top
 * @param start_pc            PC to to be used to start the stack trace or NULL
 *                            if caller is to be used as start PC
 * @param start_frame_pointer frame pointer of the top-most frame of the call chain
 *                            to unwind, or NULL if unwinding stack of the caller.
 * @param task_p              Pointer to the task for which the stack trace is to
 *                            be captured, or NULL if task is the current task.
 * @param trace_buff          Area to store the captured stack trace
 * @param num_entries_p       Area to hold the number of stack trace entries,
 *                            upon return
 */
void stack_trace_capture(uint_fast8_t num_entries_to_skip,
						 const void *start_pc,
						 const void *start_frame_pointer,
						 const struct rtos_task *task_p,
                         uintptr_t trace_buff[],
                         uint_fast8_t *num_entries_p)
{
    const uint32_t *frame_pointer;
    const uint32_t *stack_bottom_end_p;
    const uint32_t *stack_top_end_p;
    uintptr_t return_address;
    bool found_prev_stack_frame;
    uint8_t num_entries = *num_entries_p;

    *num_entries_p = 0;
    if (start_frame_pointer == NULL) {
		CAPTURE_ARM_FRAME_POINTER_REGISTER(frame_pointer);
		/*
		 * Another way of doing this is:
		 * frame_pointer =  __builtin_frame_address(0);
		 */
    } else {
    	frame_pointer = start_frame_pointer;
    }

    if (frame_pointer >= __StackLimit && frame_pointer < __StackTop) {
         stack_bottom_end_p = __StackTop;
         stack_top_end_p = __StackLimit;
     } else {
         if (task_p == NULL) {
			 const struct rtos_task *current_task_p = rtos_task_self();

			 stack_bottom_end_p =
                 (uint32_t *)&current_task_p->tsk_stack[APP_TASK_STACK_SIZE];
			 stack_top_end_p = (uint32_t *)current_task_p->tsk_stack;
         } else {
			 stack_bottom_end_p =
                 (uint32_t *)&task_p->tsk_stack[APP_TASK_STACK_SIZE];
			 stack_top_end_p = (uint32_t *)task_p->tsk_stack;
         }

         if (frame_pointer < stack_top_end_p ||
             frame_pointer >= stack_bottom_end_p) {
            return;
         }
    }

    if (start_pc == NULL) {
		found_prev_stack_frame = find_previous_stack_frame(NULL,
														   stack_bottom_end_p,
														   &frame_pointer,
														   &return_address);
		if (!found_prev_stack_frame) {
			return;
		}
    } else {
    	return_address = (uintptr_t)start_pc | 0x1;
    }

    unwind_execution_stack(num_entries_to_skip,
                           return_address,
                           frame_pointer,
                           stack_bottom_end_p,
                           trace_buff,
                           &num_entries);

    D_ASSERT(num_entries != 0);
    *num_entries_p = num_entries;
}
