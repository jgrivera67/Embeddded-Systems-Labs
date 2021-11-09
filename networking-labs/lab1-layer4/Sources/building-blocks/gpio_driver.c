/**
 * @file gpio_driver.c
 *
 * GPIO pin driver implementation
 *
 * @author: German Rivera
 */
#include "gpio_driver.h"
#include "pin_config.h"
#include "io_utils.h"
#include "atomic_utils.h"
#include <MK64F12.h>


/**
 * GPIO pin configuration registers for all pin ports
 */
static GPIO_Type *const g_pin_gpio_regs[NUM_PIN_PORTS] = {
    [PIN_PORT_A] = PTA,
    [PIN_PORT_B] = PTB,
    [PIN_PORT_C] = PTC,
    [PIN_PORT_D] = PTD,
    [PIN_PORT_E] = PTE,
};

/**
 * It configures a GPIO pin
 */
void gpio_configure_pin(const struct gpio_pin *gpio_pin_p, uint32_t pin_flags,
		   	   	        bool is_output)
{
    uint32_t reg_value;

    uint32_t old_primask = disable_cpu_interrupts();

    set_pin_function(&gpio_pin_p->pin_info, pin_flags);

    volatile GPIO_Type *gpio_regs_p =
        g_pin_gpio_regs[gpio_pin_p->pin_info.pin_port];

   if (is_output) {
        reg_value = READ_MMIO_REGISTER(&gpio_regs_p->PDDR);
        reg_value |= gpio_pin_p->pin_bit_mask;
        WRITE_MMIO_REGISTER(&gpio_regs_p->PDDR, reg_value);
    } else {
        reg_value = READ_MMIO_REGISTER(&gpio_regs_p->PDDR);
        reg_value &= ~gpio_pin_p->pin_bit_mask;
        WRITE_MMIO_REGISTER(&gpio_regs_p->PDDR, reg_value);
    }

    restore_cpu_interrupts(old_primask);
}


/**
 * It activates a pin output.
 * If the pin is active low, it sets pin low
 * If the pin is active high, it sets the pin high
 */
void gpio_activate_output_pin(const struct gpio_pin *gpio_pin_p)
{
    volatile GPIO_Type *gpio_regs_p =
        g_pin_gpio_regs[gpio_pin_p->pin_info.pin_port];

#   ifdef DEBUG
    uint32_t reg_value = READ_MMIO_REGISTER(&gpio_regs_p->PDDR);

    D_ASSERT(reg_value & gpio_pin_p->pin_bit_mask);
#   endif

    if (gpio_pin_p->pin_is_active_high) {
        WRITE_MMIO_REGISTER(&gpio_regs_p->PSOR, gpio_pin_p->pin_bit_mask);
    } else {
        WRITE_MMIO_REGISTER(&gpio_regs_p->PCOR, gpio_pin_p->pin_bit_mask);
    }
}


/**
 * It deactivates a pin output.
 * If the pin is active low, it sets pin high
 * If the pin is active high, it sets the pin low
 */
void gpio_deactivate_output_pin(const struct gpio_pin *gpio_pin_p)
{
    volatile GPIO_Type *gpio_regs_p =
        g_pin_gpio_regs[gpio_pin_p->pin_info.pin_port];

#   ifdef DEBUG
    uint32_t reg_value = READ_MMIO_REGISTER(&gpio_regs_p->PDDR);

    D_ASSERT(reg_value & gpio_pin_p->pin_bit_mask);
#   endif

    if (gpio_pin_p->pin_is_active_high) {
        WRITE_MMIO_REGISTER(&gpio_regs_p->PCOR, gpio_pin_p->pin_bit_mask);
    } else {
        WRITE_MMIO_REGISTER(&gpio_regs_p->PSOR, gpio_pin_p->pin_bit_mask);
    }
}


void gpio_toggle_output_pin(const struct gpio_pin *gpio_pin_p)
{
    volatile GPIO_Type *gpio_regs_p =
	g_pin_gpio_regs[gpio_pin_p->pin_info.pin_port];

#   ifdef DEBUG
    uint32_t reg_value = READ_MMIO_REGISTER(&gpio_regs_p->PDDR);

    D_ASSERT(reg_value & gpio_pin_p->pin_bit_mask);
#   endif

    WRITE_MMIO_REGISTER(&gpio_regs_p->PTOR, gpio_pin_p->pin_bit_mask);
}


bool gpio_read_input_pin(const struct gpio_pin *gpio_pin_p)
{
    volatile GPIO_Type *gpio_regs_p =
        g_pin_gpio_regs[gpio_pin_p->pin_info.pin_port];

    uint32_t reg_value = READ_MMIO_REGISTER(&gpio_regs_p->PDIR);

    bool result = ((reg_value & gpio_pin_p->pin_bit_mask) != 0);

    return result;
}


void gpio_enable_pin_irq(const struct gpio_pin *gpio_pin_p, enum gpio_pin_irq_type irq_type)
{
	uint32_t reg_value;

	const struct pin_info *pin_p = &gpio_pin_p->pin_info;
    PORT_Type *port_regs_p = g_pin_port_regs[pin_p->pin_port];

    reg_value = READ_MMIO_REGISTER(&port_regs_p->PCR[pin_p->pin_index]);

    SET_BIT_FIELD(reg_value, PORT_PCR_IRQC_MASK, PORT_PCR_IRQC_SHIFT, irq_type);
    WRITE_MMIO_REGISTER(&port_regs_p->PCR[pin_p->pin_index], reg_value);
}


void gpio_disable_pin_irq(const struct gpio_pin *gpio_pin_p)
{
	uint32_t reg_value;

	const struct pin_info *pin_p = &gpio_pin_p->pin_info;
    PORT_Type *port_regs_p = g_pin_port_regs[pin_p->pin_port];

    reg_value = READ_MMIO_REGISTER(&port_regs_p->PCR[pin_p->pin_index]);

    SET_BIT_FIELD(reg_value, PORT_PCR_IRQC_MASK, PORT_PCR_IRQC_SHIFT, 0x0);
    WRITE_MMIO_REGISTER(&port_regs_p->PCR[pin_p->pin_index], reg_value);
}


/**
 * Clear the interrupt for an input pin configured to generate interrupts.
 *
 * NOTE: If the pin is configured for a level sensitive interrupt and the
 * pin remains asserted, then the flag is set again immediately after it is
 * cleared.
 *
 */
void gpio_clear_pin_irq(const struct gpio_pin *gpio_pin_p)
{
	const struct pin_info *pin_p = &gpio_pin_p->pin_info;
    PORT_Type *port_regs_p = g_pin_port_regs[pin_p->pin_port];

    D_ASSERT(READ_MMIO_REGISTER(&port_regs_p->ISFR) & BIT(pin_p->pin_index));
    WRITE_MMIO_REGISTER(&port_regs_p->ISFR, BIT(pin_p->pin_index));
}

