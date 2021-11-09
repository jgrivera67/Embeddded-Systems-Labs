/**
 * @file pin_config.c
 *
 * Pin configurator implementation
 *
 * @author: German Rivera
 */
#include "pin_config.h"
#include "io_utils.h"
#include "runtime_checks.h"
#include <stddef.h>

/**
 * PORT pin configuration registers for all pin ports
 */
PORT_Type *const g_pin_port_regs[NUM_PIN_PORTS] = {
    [PIN_PORT_A] = PORTA,
    [PIN_PORT_B] = PORTB,
    [PIN_PORT_C] = PORTC,
    [PIN_PORT_D] = PORTD,
    [PIN_PORT_E] = PORTE,
};

/**
 * Matrix to keep track of what pins are currently in use. If a pin is not in
 * use (set_pin_function() has not been called for it), its entry is NULL.
 */
static const struct pin_info *g_pins_in_use_map[NUM_PIN_PORTS][NUM_PINS_PER_PORT];

void pin_config_init(void)
{
    uint32_t reg_value;

    /*
     * Enable clocks for all GPIO ports. These have to be enabled to configure
     * pin muxing.
     */
    reg_value = READ_MMIO_REGISTER(&SIM->SCGC5);
    reg_value |= (SIM_SCGC5_PORTA_MASK |
                  SIM_SCGC5_PORTB_MASK |
                  SIM_SCGC5_PORTC_MASK |
                  SIM_SCGC5_PORTD_MASK |
                  SIM_SCGC5_PORTE_MASK);
    WRITE_MMIO_REGISTER(&SIM->SCGC5, reg_value);
}


void set_pin_function(const struct pin_info *pin_p, uint32_t pin_flags)
{
    const struct pin_info **pins_in_use_entry_p =
        &g_pins_in_use_map[pin_p->pin_port][pin_p->pin_index];

    if (*pins_in_use_entry_p != NULL) {
        error_t error = CAPTURE_ERROR("Pin already allocated", pin_p->pin_port,
                                      pin_p->pin_index);

        fatal_error_handler(error);
    }

    volatile PORT_Type *port_regs_p = g_pin_port_regs[pin_p->pin_port];

    WRITE_MMIO_REGISTER(&port_regs_p->PCR[pin_p->pin_index],
                        PORT_PCR_MUX(pin_p->pin_function) | pin_flags);

    *pins_in_use_entry_p = pin_p;
}




