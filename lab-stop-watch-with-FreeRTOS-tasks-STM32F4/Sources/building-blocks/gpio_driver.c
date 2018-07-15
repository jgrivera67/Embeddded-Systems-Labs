/**
 * @file gpio_driver.c
 *
 * GPIO pin driver implementation
 *
 * @author: German Rivera
 */
#include "gpio_driver.h"
#include "atomic_utils.h"

/**
 * It configures a GPIO pin
 */
void gpio_configure_pin(const struct gpio_pin *gpio_pin_p, uint32_t pin_flags)
{
    uint32_t old_primask = disable_cpu_interrupts();

    set_pin_function(&gpio_pin_p->pin_info, pin_flags);

    restore_cpu_interrupts(old_primask);
}


/**
 * It activates a pin output.
 * If the pin is active low, it sets pin low
 * If the pin is active high, it sets the pin high
 */
void gpio_activate_output_pin(const struct gpio_pin *gpio_pin_p)
{
    volatile GPIO_TypeDef *gpio_regs_p =
        g_pin_port_regs[gpio_pin_p->pin_info.pin_port];

    if (gpio_pin_p->pin_is_active_high) {
        gpio_regs_p->ODR |= gpio_pin_p->pin_bit_mask;
    } else {
        gpio_regs_p->ODR &= ~gpio_pin_p->pin_bit_mask;
    }
}


/**
 * It deactivates a pin output.
 * If the pin is active low, it sets pin high
 * If the pin is active high, it sets the pin low
 */
void gpio_deactivate_output_pin(const struct gpio_pin *gpio_pin_p)
{
    volatile GPIO_TypeDef *gpio_regs_p =
        g_pin_port_regs[gpio_pin_p->pin_info.pin_port];

    if (gpio_pin_p->pin_is_active_high) {
        gpio_regs_p->ODR &= ~gpio_pin_p->pin_bit_mask;
    } else {
        gpio_regs_p->ODR |= gpio_pin_p->pin_bit_mask;
    }
}


void gpio_toggle_output_pin(const struct gpio_pin *gpio_pin_p)
{
    volatile GPIO_TypeDef *gpio_regs_p =
	g_pin_port_regs[gpio_pin_p->pin_info.pin_port];

    gpio_regs_p->ODR ^= gpio_pin_p->pin_bit_mask;
}


bool gpio_read_input_pin(const struct gpio_pin *gpio_pin_p)
{
    volatile GPIO_TypeDef *gpio_regs_p =
        g_pin_port_regs[gpio_pin_p->pin_info.pin_port];

    uint32_t reg_value = gpio_regs_p->IDR;

    bool result = ((reg_value & gpio_pin_p->pin_bit_mask) != 0);

    return result;
}

#if 0
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
#endif /* #if 0 */
