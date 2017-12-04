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
#if 0 // TODO
#include "system_clocks.h"
#else
#include "clock_config.h"
#endif
#include <stdint.h>

extern void __libc_init_array(void); /* function provided by libc */

extern void main(void);

/**
 * Startup execution time in CPU cycles
 */
static uint32_t g_startup_time_cycles = 0;


static void init_c_global_and_static_variables(void)
{
    extern uint32_t __VECTOR_TABLE[];
    extern uint32_t __VECTOR_RAM[];
    extern uint32_t __DATA_ROM[];
    extern uint32_t __DATA_RAM[];
    extern uint32_t __DATA_END[];
    extern uint32_t __START_BSS[];
    extern uint32_t __END_BSS[];
    extern uint32_t __RAM_VECTOR_TABLE_SIZE_BYTES[];
    uint32_t size;

    if (__VECTOR_RAM != __VECTOR_TABLE) {
        /*
         * Copy interrupt vector table from flash to SRAM:
         */

        /*
         *  Interpret linker label '__RAM_VECTOR_TABLE_SIZE_BYTES' as a value
         *  instead of an address:
         */
        size = (uintptr_t)(__RAM_VECTOR_TABLE_SIZE_BYTES);
    	memcpy32(__VECTOR_RAM, __VECTOR_TABLE, size);

        /* Point the VTOR to the position of vector table in SRAM */
        SCB->VTOR = (uint32_t)__VECTOR_RAM;
    } else {
        /* Point the VTOR to the position of vector table in flash */
        SCB->VTOR = (uint32_t)__VECTOR_TABLE;
    }

    /*
     * Copy initialized C global/static variables from flash to SRAM
     */
    size = (uintptr_t)__DATA_END - (uintptr_t)__DATA_ROM;
   	memcpy32(__DATA_RAM, __DATA_ROM, size);

    /*
     * Initialize uninitialized C global/static variables to 0s:
     */
    size = (uintptr_t)__END_BSS - (uintptr_t)__START_BSS;

   	memset32(__START_BSS, 0, size);
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
#if 0 // TODO
    system_clocks_init();
#endif
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
    uintptr_t initializers_size = __data_end__ - __data_start__;

    D_ASSERT((uintptr_t)__etext <= MCU_FLASH_BASE_ADDR + MCU_FLASH_SIZE);
    D_ASSERT(__data_start__ < __data_end__);

    return ((uintptr_t)__etext - MCU_FLASH_BASE_ADDR) + initializers_size;
}


/**
 * Returns the amount of SRAM used by the program, including the program's stack.
 * It is assumed that the program does not use malloc(). So the heap is ignored.
 *
 * @return amount of SRAM used in bytes
 */
uint32_t get_sram_used(void)
{
    D_ASSERT(__bss_end__ <= __StackLimit);
    D_ASSERT(__StackLimit < __StackTop);

    uint32_t prog_stack_size = (uintptr_t)__StackTop - (uintptr_t)__StackLimit;

    return ((uintptr_t)__bss_end__ - MCU_SRAM_BASE_ADDR) + prog_stack_size;
}
