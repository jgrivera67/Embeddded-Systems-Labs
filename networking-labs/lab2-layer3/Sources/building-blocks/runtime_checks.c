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
#include "runtime_log.h"

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

    color_led_set(LED_COLOR_RED);

    console_printf("\n*** Assertion '%s' failed in %s() at %s:%d ***\n",
                   cond_str, func_name, file_name, line);

    ERROR_PRINTF("Assertion failed at %s:%d\n", file_name, line);

    /*
     * If running under the debugger, break into it. Otherwise,
     * lockup the CPU.
     */
    __BKPT(0);

    g_handling_assert_failure = false;

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

#if 0 // ???
    console_printf("ERROR: %s (arg1: %#x, arg2: %#x, location: %#x)\n",
                   error_description, arg1, arg2, error_address);
#else
    ERROR_PRINTF("ERROR: %s (arg1: %#x, arg2: %#x, location: %#x)\n",
                 error_description, arg1, arg2, error_address);
#endif

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

    g_handling_fatal_error = true;
    color_led_set(LED_COLOR_RED);
    console_printf("\n*** Fatal error %#x ***\n", error);

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
