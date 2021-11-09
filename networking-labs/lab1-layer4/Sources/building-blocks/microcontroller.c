/**
 * @file microcontroller.c
 *
 * Microcontroller services
 *
 * @author: German Rivera
 */
#include "microcontroller.h"
#include "io_utils.h"

/**
 * Trigger software reset  for the microcontroller
 */
void reset_cpu(void)
{
    __disable_irq();

    NVIC_SystemReset();
}


enum cpu_reset_causes find_cpu_reset_cause(void)
{
	uint32_t reg_value;
	enum cpu_reset_causes reset_cause = INVALID_RESET_CAUSE;

    reg_value = READ_MMIO_REGISTER(&RCM->SRS0);
    if (reg_value & RCM_SRS0_POR_MASK) {
        reset_cause = POWER_ON_RESET;
    } else if (reg_value & RCM_SRS0_PIN_MASK) {
        reset_cause = EXTERNAL_PIN_RESET;
    } else if (reg_value & RCM_SRS0_WDOG_MASK) {
        reset_cause = WATCHDOG_RESET;
    } else if (reg_value != 0) {
        reset_cause = OTHER_HW_REASON_RESET;
    } else {
        reg_value = READ_MMIO_REGISTER(&RCM->SRS1);
        if (reg_value & RCM_SRS1_SW_MASK) {
            reset_cause = SOFTWARE_RESET;
        } else if (reg_value & RCM_SRS1_MDM_AP_MASK) {
            reset_cause = EXTERNAL_DEBUGGER_RESET;
        } else if (reg_value & RCM_SRS1_LOCKUP_MASK) {
            reset_cause = LOCKUP_EVENT_RESET;
        } else if (reg_value & RCM_SRS1_SACKERR_MASK) {
            reset_cause = STOP_ACK_ERROR_RESET;
        }
    }

    return reset_cause;
}

