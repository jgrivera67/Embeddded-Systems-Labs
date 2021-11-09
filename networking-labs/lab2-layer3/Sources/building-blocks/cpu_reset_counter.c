/**
 * @file cpu_reset_counter.c
 *
 * CPU reset counter implementation
 *
 * @author: German Rivera
 */
#include "cpu_reset_counter.h"
#include "mem_utils.h"
#include "runtime_checks.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/**
 * CPU reset counter object type
 *
 * NOTE: Since SRAM will contain garbage after power-cycling the
 * microcontroller, we need to use a checksum to be able to
 * differentiate garbage from valid data from a previous reset.
 */
struct cpu_reset_counter {
	/**
	 * Number of CPU resets since last power-cycle
	 */
	uint32_t count;

	/**
	 * Last checksum calculated for this struct excluding
	 * this field.
	 */
	uint32_t checksum;
};

/**
 * CPU reset counter object
 *
 * NOTE: This is not a regular C global variable, as it is not
 * in the '.data' section nor in  the '.bss' section (see linker script).
 */
static struct cpu_reset_counter
	__attribute__ ((section(".cpu_reset_counter"))) g_cpu_reset_counter;


/**
 * Computes the checksum for the CPU reset counter. If the checksum does not
 * match, g_cpu_reset_counter contains garbage.
 *
 * NOTE: SRAM contains garbage after power-cycling the microcontroller.
 * However, it keeps its values across resets.
 *
 * @return true, if CPU reset counter was valid (checksum matched)
 *
 * @return false, if CPU reset count was invalid (checksum did not match)
 */
bool valid_cpu_reset_counter(void)
{
	uint32_t crc = mem_checksum(&g_cpu_reset_counter.count,
								sizeof(g_cpu_reset_counter.count));

	return (crc == g_cpu_reset_counter.checksum);
}


/**
 * Initialize CPU reset counter t 0
 */
void initialize_cpu_reset_counter(void)
{
    g_cpu_reset_counter.count = 0;
	g_cpu_reset_counter.checksum = mem_checksum(&g_cpu_reset_counter.count,
        			 	 	 	 	 	 	    sizeof(g_cpu_reset_counter.count));
}


/**
 * Increment the CPU reset counter.
 */
void increment_cpu_reset_counter(void)
{
	g_cpu_reset_counter.count ++;
	g_cpu_reset_counter.checksum = mem_checksum(&g_cpu_reset_counter.count,
        			 	 	 	 	 	 	    sizeof(g_cpu_reset_counter.count));
}


/**
 * Retrieves the current value of the CPU reset counter
 *
 * @return current CPU reset count
 */
uint32_t read_cpu_reset_counter(void)
{
	D_ASSERT(valid_cpu_reset_counter());

	return g_cpu_reset_counter.count;
}
