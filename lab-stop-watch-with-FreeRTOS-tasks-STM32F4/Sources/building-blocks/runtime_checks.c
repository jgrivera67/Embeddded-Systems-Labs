/**
 * @file runtime_checks.c
 *
 * Runtime checks services implementation
 *
 * @author German Rivera
 */
#include "runtime_checks.h"
#include "color_led.h"
#include "serial_console.h"
#include "atomic_utils.h"
#include "microcontroller.h"
#include "stack_trace.h"
#include "printf_utils.h"
#include <stddef.h>
#include "memory_protection_unit.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void emergency_putc(void *putchar_arg_p, uint8_t c)
{
	do {
	} while (!(USART1->SR & USART_SR_RXNE));

	USART1->DR = c;
}

#pragma GCC diagnostic pop

/**
 * printf service that sends emergency output directly to the serial porrt
 * doing polling
 */
void emergency_printf(const char  *fmt_s, ...)
{
    va_list  va;
    bool mpu_was_enabled = mpu_disable();

    va_start(va, fmt_s);
    embedded_vprintf(emergency_putc, NULL, fmt_s, va);
    va_end(va);

    if (mpu_was_enabled) {
    	mpu_enable();
    }
}


void print_stack_trace(uint_fast8_t num_entries_to_skip,
		               const void *start_pc,
					   const void *start_frame_pointer,
					   const struct rtos_task *task_p)
{
    uintptr_t trace_buff[10];
    uint_fast8_t num_trace_entries = sizeof(trace_buff) / sizeof(trace_buff[0]);

    stack_trace_capture(num_entries_to_skip,
    		            start_pc,
						start_frame_pointer,
						task_p,
                        trace_buff,
                        &num_trace_entries);

    for (uint_fast8_t i = 0; i < num_trace_entries; i ++) {
        emergency_printf("\t%#x\n", trace_buff[i]);
    }
}

/**
 * Handles debug-assertion violations
 *
 * @param cond_str        assertion condition string
 * @param func_name        Name of the function that contains the assertion
 * @param file_name        Source file where the assertion is located
 * @param line            text line number where the assertion is located
 */
void debug_assert_failure(const char *cond_str, const char *func_name,
                          const char *file_name, int line)
{
	bool old_writable = set_writable_background_region(true);

    static bool g_handling_assert_failure = false;

    uint32_t old_intmask = disable_cpu_interrupts();

    /*
     * Detect unwanted recursive call:
     */
    if (g_handling_assert_failure) {
        restore_cpu_interrupts(old_intmask);
        return;
    }

    g_handling_assert_failure = true;

#if 0 //???
    color_led_set(LED_COLOR_RED);
#endif

    emergency_printf("\n*** Assertion '%s' failed in %s() at %s:%d ***\n",
                     cond_str, func_name, file_name, line);

    print_stack_trace(2, NULL, NULL, NULL);


#if 1
    /*
     * If running under the debugger, break into it. Otherwise,
     * lockup the CPU.
     */
    __BKPT(0);
#endif

    g_handling_assert_failure = false;
    (void)set_writable_background_region(old_writable);

    /*
     * Trap CPU in a dummy infinte loop
     */
    for ( ; ; )
        ;
}


/**
 * Captures a runtime error
 *
 * @param error_description: error message
 * @param arg1: value 1 associated with the error
 * @param arg2: value 2 associated with the error
 *
 * @return Unique error code that encodes the address where this function
 *         was invoked.
 */
error_t capture_error(const char *error_description,
                      uintptr_t arg1,
                      uintptr_t arg2)
{
    uintptr_t error_address;
    void *return_address = __builtin_return_address(0);

    /*
     * Get call site address:
     */
    error_address = GET_CALL_ADDRESS(return_address);

    /*
     * Log error:
     */
    uint32_t old_primask = disable_cpu_interrupts();

    emergency_printf("ERROR: %s (arg1: %#x, arg2: %#x, location: %#x)\n",
                     error_description, arg1, arg2, error_address);

    restore_cpu_interrupts(old_primask);
    return (error_t)error_address;
}


void fatal_error_handler(error_t error)
{
    static bool g_handling_fatal_error = false;

    /*
     * Detect unwanted recursive call:
     */
    if (g_handling_fatal_error)
        return;

    /*
     * Block all further interrupts:
     */
    (void)disable_cpu_interrupts();
    (void)mpu_disable();
    g_handling_fatal_error = true;
#if 0 //???
    color_led_set(LED_COLOR_RED);
#endif
    emergency_printf("\n*** Fatal error %#x ***\n", error);
    print_stack_trace(2, NULL, NULL, NULL);

    /*
     * If running under the debugger, break into it. Otherwise,
     * lockup the CPU.
     */
    __BKPT(0);

    /*
     * Trap CPU in a dummy infinite loop
     */
    for ( ; ; )
        ;
}
