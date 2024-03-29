/**
 * embedded_debug.c - Embedded debugging facilities
 *
 * @author: German Rivera
 */
#include "embedded_debug.h"
#include <fsl_debug_console.h>

void capture_assert_failure(const char *cond_str, const char *func_name,
						    const char *file_name, int line)
{
	PRINTF("ASSERT: '%s' failed in %s() at %s:%d\n",
			cond_str, func_name, file_name, line);

    /*
     * Trap execution in this infinite loop:
     */
	for ( ; ; ) {
        /* TODO: Blink RGB LED in some distinctive pattern */
	}
}

