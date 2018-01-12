/**
 * @file hw_timer.h
 *
 * Hardware timer driver interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_HW_TIMER_DRIVER_H_
#define SOURCES_BUILDING_BLOCKS_HW_TIMER_DRIVER_H_

#include <stdint.h>
#include "runtime_checks.h"
#include "microcontroller.h"

typedef void hw_timer_callback_t(void *arg);

/**
 * Immutable data of the hardware timer device (to be placed in flash)
 */
struct hw_timer_device {
#   define HW_TIMER_SIGNATURE  GEN_SIGNATURE('T', 'I', 'M', 'E')
    uint32_t signature;

    /**
     * Pointer to MMIO registers for the device
     */
    LPTMR_Type *mmio_registers_p;

    /**
     * Pointer to device data in SRAM
     */
    struct hw_timer_device_var *var_p;
};


void hw_timer_init(const struct hw_timer_device *hw_timer_p,
				   uint16_t timer_period_ms,
		           hw_timer_callback_t *timer_callback_func,
				   void *timer_callback_arg);

extern const struct hw_timer_device g_hw_timer0;

#endif /* SOURCES_BUILDING_BLOCKS_HW_TIMER_DRIVER_H_ */
