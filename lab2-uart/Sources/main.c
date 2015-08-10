 /*
  * Embedded System Lab 2 - UART
  */

#include <fsl_device_registers.h>
#include <board.h>
#include <fsl_debug_console.h>
#include "embedded_debug.h"

int main(void)
{
    hardware_init();
    dbg_uart_init();

    PRINTF("lab2 - UART: hello world\n");

    /*  Never leave main */
    for ( ; ; ) {
        /*
         * TODO: Read characters from the UART and echo them back
         */
    }

    ASSERT(false);
    return 0;
}

