/**
 * @file cortex_m_startup.c
 *
 * Startup code for ARM Cortex-M processors
 *
 * @author: German Rivera
 */
#include "cortex_m_startup.h"
#include "cpu_reset_counter.h"
#include "runtime_checks.h"
#include "mem_utils.h"
#include "microcontroller.h"
#include "interrupt_vector_table.h"
#include "time_utils.h"
#include "watchdog.h"
#include "runtime_log.h"
#include "system_clocks.h"
#include <stdint.h>

extern void __libc_init_array(void); /* function provided by libc */

extern void main(void);

/**
 * Startup execution time in CPU cycles
 */
static uint32_t g_startup_time_cycles = 0;


static void init_c_global_and_static_variables(void)
{
    uint32_t size;

    /* VTOR points to vector table in flash */
    D_ASSERT(SCB->VTOR == (uint32_t)g_interrupt_vector_table);

    /*
     * Copy initialized C global/static variables from flash to SRAM
     */
    size = (uintptr_t)__data_end__ - (uintptr_t)__data_start__;
    memcpy32(__data_start__, _sidata, size);

    /*
     * Initialize uninitialized C global/static variables to 0s:
     */
    size = (uintptr_t)__bss_end__ - (uintptr_t)__bss_start__;

    memset32(__bss_start__, 0, size);
}


/**
 * Reset exception handler
 */
void Reset_Handler(void)
{
    uint32_t begin_cycles;
    uint32_t end_cycles;

    /*
     * Disable interrupts at the CPU:
     *
     * NOTE: On ARM Cortex-M processors interrupts are in enabled state when
     * entering the reset handler (the same is true for other exceptions and
     * interrupts).
     */
    __disable_irq();

    /*
     * CAUTION: C global/static variables cannot be accessed before the
     * init_c_global_and_static_variables() call below.
     */

    /*
     * Initialize watchdog:
     *
     * NOTE: This has to be done first, otherwise the watchdog will fire.
     */
    watchdog_init();

    /*
     * Initialize system clocks:
     */
    system_clocks_init();

    /*
     * Initialize CPU cycle counter used to measure execution time:
     */
    init_cpu_clock_cycles_counter();

    begin_cycles = get_cpu_clock_cycles();

    /*
     * Update CPU reset counter:
     */
    if (valid_cpu_reset_counter()) {
        increment_cpu_reset_counter();
    } else {
        initialize_cpu_reset_counter();
    }

    /*
     * Initialize data sections in SRAM:
     */
    init_c_global_and_static_variables();

     /*
      * NOTE: C global and static variables can only be accessed after this point
      */

#if 0
    /*
     * Run C++ static constructors:
     */
    __libc_init_array();
#endif

    /*
     * NOTE: C++ global/static objects can only be accessed after this point
     */

    end_cycles = get_cpu_clock_cycles();
    g_startup_time_cycles = cpu_clock_cycles_diff(begin_cycles, end_cycles);

    /*
     * Restart wathdog timer to prevent a reset before the RTOS idle task gets
     * the chance to run. (The idle task restarts the watchdog timer)
     */
#if 0 //???
    watchdog_restart();
#endif

    /*
     * Re-enable interrupts at the CPU:
     */
    __enable_irq();

    /*
     * Invoke the program's main() function:
     */
    main();

    /*
     * We should never return here:
     */
    D_ASSERT(false);

    /*
     * Trap CPU in a dummy infinite loop:
     */
    for ( ; ; )
        ;
}


uint32_t get_starup_time_us(void)
{
    return CPU_CLOCK_CYCLES_TO_MICROSECONDS(g_startup_time_cycles);
}


/**
 * Returns the amount of flash used by the program
 *
 * @return amount of flash used in bytes
 */
uint32_t get_flash_used(void)
{
    D_ASSERT(__data_start__ < __data_end__);
    uintptr_t initializers_size = (uintptr_t)__data_end__ - (uintptr_t)__data_start__;

    D_ASSERT((uintptr_t)_sidata <= MCU_FLASH_BASE_ADDR + MCU_FLASH_SIZE);

    return ((uintptr_t)_sidata - MCU_FLASH_BASE_ADDR) + initializers_size;
}


/**
 * Returns the amount of SRAM used by the program, including the program's stack.
 * It is assumed that the program does not use malloc(). So the heap is ignored.
 *
 * @return amount of SRAM used in bytes
 */
uint32_t get_sram_used(void)
{
    D_ASSERT(__bss_end__ <= __stack_start__);
    D_ASSERT(__stack_start__ < __stack_end__);

    uint32_t prog_stack_size = (uintptr_t)__stack_end__ - (uintptr_t)__stack_start__;

    return ((uintptr_t)__bss_end__ - MCU_SRAM_BASE_ADDR) + prog_stack_size;
}
