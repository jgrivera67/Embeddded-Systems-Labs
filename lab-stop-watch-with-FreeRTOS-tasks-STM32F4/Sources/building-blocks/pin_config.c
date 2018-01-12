/**
 * @file pin_config.c
 *
 * Pin configurator implementation
 *
 * @author: German Rivera
 */
#include "pin_config.h"
#include "io_utils.h"
#include "compile_time_checks.h"
#include "runtime_checks.h"
#include "mem_utils.h"
#include "atomic_utils.h"
#include <stddef.h>

/**
 * PORT pin configuration registers for all pin ports
 */
GPIO_TypeDef *const g_pin_port_regs[NUM_PIN_PORTS] = {
    [PIN_PORT_A] = GPIOA,
    [PIN_PORT_B] = GPIOB,
    [PIN_PORT_C] = GPIOC,
    [PIN_PORT_D] = GPIOD,
    [PIN_PORT_E] = GPIOE,
    [PIN_PORT_H] = GPIOH,
};

/**
 * Matrix to keep track of what pins are currently in use. If a pin is not in
 * use (set_pin_function() has not been called for it), its entry is NULL.
 */
static const struct pin_info *g_pins_in_use_map[NUM_PIN_PORTS][NUM_PINS_PER_PORT];

static bool g_pin_ports_enabled[NUM_PIN_PORTS] = { false };

void pin_config_init(void)
{
    uint32_t reg_value;

    /*
     * Disable clocks for all GPIO ports to save power:
     */
    reg_value = RCC->AHB1ENR;
    reg_value &= ~(RCC_AHB1ENR_GPIOAEN |
	           RCC_AHB1ENR_GPIOBEN |
	           RCC_AHB1ENR_GPIOCEN |
	           RCC_AHB1ENR_GPIODEN |
	           RCC_AHB1ENR_GPIOEEN |
	           RCC_AHB1ENR_GPIOHEN);
    RCC->AHB1ENR = reg_value;

    for (pin_port_t i = 0; i < NUM_PIN_PORTS; i++) {
	g_pin_ports_enabled[i] = false;
    }
}


static void enable_pin_port_clock(pin_port_t pin_port)
{
    static const uint32_t ahb1enr_masks[] = {
	[PIN_PORT_A] = RCC_AHB1ENR_GPIOAEN,
	[PIN_PORT_B] = RCC_AHB1ENR_GPIOBEN,
	[PIN_PORT_C] = RCC_AHB1ENR_GPIOCEN,
	[PIN_PORT_D] = RCC_AHB1ENR_GPIODEN,
	[PIN_PORT_E] = RCC_AHB1ENR_GPIOEEN,
	[PIN_PORT_H] = RCC_AHB1ENR_GPIOHEN,
    };
    C_ASSERT(ARRAY_SIZE(ahb1enr_masks) == NUM_PIN_PORTS);

    uint32_t reg_value;
    uint32_t int_mask = disable_cpu_interrupts();

    if (g_pin_ports_enabled[pin_port]) {
	restore_cpu_interrupts(int_mask);
    }

    g_pin_ports_enabled[pin_port] = true;
    restore_cpu_interrupts(int_mask);

    /*
     * Enable clock for corresponding GPIO port peripheral:
     */
    reg_value = RCC->AHB1ENR;
    reg_value |= ahb1enr_masks[pin_port];
    RCC->AHB1ENR = reg_value;

    do {
	reg_value = RCC->AHB1ENR;
    } while ((reg_value & ahb1enr_masks[pin_port]) == 0);
}


void set_pin_function(const struct pin_info *pin_p, uint32_t pin_flags)
{
    struct bit_field {
	uint32_t mask;
	uint8_t shift;
    };

    static const struct bit_field moder_bit_fields[] = {
	[0] = { GPIO_MODER_MODER0_Msk, GPIO_MODER_MODER0_Pos },
	[1] = { GPIO_MODER_MODER1_Msk, GPIO_MODER_MODER1_Pos },
	[2] = { GPIO_MODER_MODER2_Msk, GPIO_MODER_MODER2_Pos },
	[3] = { GPIO_MODER_MODER3_Msk, GPIO_MODER_MODER3_Pos },
	[4] = { GPIO_MODER_MODER4_Msk, GPIO_MODER_MODER4_Pos },
	[5] = { GPIO_MODER_MODER5_Msk, GPIO_MODER_MODER5_Pos },
	[6] = { GPIO_MODER_MODER6_Msk, GPIO_MODER_MODER6_Pos },
	[7] = { GPIO_MODER_MODER7_Msk, GPIO_MODER_MODER7_Pos },
	[8] = { GPIO_MODER_MODER8_Msk, GPIO_MODER_MODER8_Pos },
	[9] = { GPIO_MODER_MODER9_Msk, GPIO_MODER_MODER9_Pos },
	[10] = { GPIO_MODER_MODER10_Msk, GPIO_MODER_MODER10_Pos },
	[11] = { GPIO_MODER_MODER11_Msk, GPIO_MODER_MODER11_Pos },
	[12] = { GPIO_MODER_MODER12_Msk, GPIO_MODER_MODER12_Pos },
	[13] = { GPIO_MODER_MODER13_Msk, GPIO_MODER_MODER13_Pos },
	[14] = { GPIO_MODER_MODER14_Msk, GPIO_MODER_MODER14_Pos },
	[15] = { GPIO_MODER_MODER15_Msk, GPIO_MODER_MODER15_Pos }
    };
    C_ASSERT(ARRAY_SIZE(moder_bit_fields) == NUM_PINS_PER_PORT);

    static const struct bit_field afr_bit_fields[] = {
	[0] = { GPIO_AFRL_AFSEL0_Msk, GPIO_AFRL_AFSEL0_Pos },
	[1] = { GPIO_AFRL_AFSEL1_Msk, GPIO_AFRL_AFSEL1_Pos },
	[2] = { GPIO_AFRL_AFSEL2_Msk, GPIO_AFRL_AFSEL2_Pos },
	[3] = { GPIO_AFRL_AFSEL3_Msk, GPIO_AFRL_AFSEL3_Pos },
	[4] = { GPIO_AFRL_AFSEL4_Msk, GPIO_AFRL_AFSEL4_Pos },
	[5] = { GPIO_AFRL_AFSEL5_Msk, GPIO_AFRL_AFSEL5_Pos },
	[6] = { GPIO_AFRL_AFSEL6_Msk, GPIO_AFRL_AFSEL6_Pos },
	[7] = { GPIO_AFRL_AFSEL7_Msk, GPIO_AFRL_AFSEL7_Pos },
	[8] = { GPIO_AFRH_AFSEL8_Msk, GPIO_AFRH_AFSEL8_Pos },
	[9] = { GPIO_AFRH_AFSEL9_Msk, GPIO_AFRH_AFSEL9_Pos },
	[10] = { GPIO_AFRH_AFSEL10_Msk, GPIO_AFRH_AFSEL10_Pos },
	[11] = { GPIO_AFRH_AFSEL11_Msk, GPIO_AFRH_AFSEL11_Pos },
	[12] = { GPIO_AFRH_AFSEL12_Msk, GPIO_AFRH_AFSEL12_Pos },
	[13] = { GPIO_AFRH_AFSEL13_Msk, GPIO_AFRH_AFSEL13_Pos },
	[14] = { GPIO_AFRH_AFSEL14_Msk, GPIO_AFRH_AFSEL14_Pos },
	[15] = { GPIO_AFRH_AFSEL15_Msk, GPIO_AFRH_AFSEL15_Pos },
    };
    C_ASSERT(ARRAY_SIZE(afr_bit_fields) == NUM_PINS_PER_PORT);

    D_ASSERT(pin_p->pin_index < NUM_PINS_PER_PORT);
    D_ASSERT(pin_flags == 0);

    /*
     * Check that pin is not already in use:
     */
    uint32_t int_mask = disable_cpu_interrupts();
    const struct pin_info **pins_in_use_entry_p =
        &g_pins_in_use_map[pin_p->pin_port][pin_p->pin_index];

    if (*pins_in_use_entry_p != NULL) {
        error_t error = CAPTURE_ERROR("Pin already allocated", pin_p->pin_port,
                                      pin_p->pin_index);

        fatal_error_handler(error);
	/*unreachable*/
    }

    *pins_in_use_entry_p = pin_p;

    restore_cpu_interrupts(int_mask);

    /*
     * Enable pin port clock if necessary:
     */
    enable_pin_port_clock(pin_p->pin_port);

    volatile GPIO_TypeDef *port_regs_p = g_pin_port_regs[pin_p->pin_port];
    uint32_t reg_value;

    reg_value = port_regs_p->MODER;
    SET_BIT_FIELD(reg_value,
	          moder_bit_fields[pin_p->pin_index].mask,
	          moder_bit_fields[pin_p->pin_index].shift,
	          pin_p->pin_mode);
    port_regs_p->MODER = reg_value;

    if (pin_p->pin_mode == PIN_MODE_ALTERNATE_FUNCTION) {
	uint_fast8_t afr_index = pin_p->pin_index / 8;

	reg_value = port_regs_p->AFR[afr_index];
	SET_BIT_FIELD(reg_value,
		      afr_bit_fields[pin_p->pin_index].mask,
		      afr_bit_fields[pin_p->pin_index].shift,
		      pin_p->pin_alternate_function);
	port_regs_p->AFR[afr_index] = reg_value;
    }
}

