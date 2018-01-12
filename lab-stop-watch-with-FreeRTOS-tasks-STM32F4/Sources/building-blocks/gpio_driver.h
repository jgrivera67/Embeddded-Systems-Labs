/**
 * @file gpio_driver.c
 *
 * GPIO pin driver interface
 *
 * @author: German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_GPIO_DRIVER_H_
#define SOURCES_BUILDING_BLOCKS_GPIO_DRIVER_H_

#include "pin_config.h"
#include "io_utils.h"
#include <stdbool.h>

/**
 * Initializer for a gpio_pin structure
 */
#define GPIO_PIN_INITIALIZER(_pin_port, _pin_index, _pin_function,             \
                             _pin_is_active_high)                              \
        {                                                                      \
            .pin_info = PIN_INITIALIZER(_pin_port, _pin_index, _pin_function), \
            .pin_bit_mask = BIT(_pin_index),                                   \
            .pin_is_active_high = (_pin_is_active_high),                       \
        }

/**
 * GPIO pin
 */
struct gpio_pin {
    struct pin_info pin_info;
    uint32_t pin_bit_mask;
    uint8_t pin_is_active_high; /*  false - low, true - high */
};

enum gpio_pin_irq_type {
    GPIO_PIN_IRQ_WHEN_LOGIC_ZERO =    0x8,
    GPIO_PIN_IRQ_ON_RISING_EDGE =     0x9,
    GPIO_PIN_IRQ_ON_FALLING_EDGE =    0xA,
    GPIO_PIN_IRQ_ON_EITHER_EDGE =     0xB,
    GPIO_PIN_IRQ_WHEN_LOGIC_ONE =     0xC,
};

void gpio_configure_pin(const struct gpio_pin *gpio_pin_p, uint32_t pin_flags,
                        bool is_output);

void gpio_activate_output_pin(const struct gpio_pin *gpio_pin_p);

void gpio_deactivate_output_pin(const struct gpio_pin *gpio_pin_p);

void gpio_toggle_output_pin(const struct gpio_pin *gpio_pin_p);

bool gpio_read_input_pin(const struct gpio_pin *gpio_pin_p);

void gpio_enable_pin_irq(const struct gpio_pin *gpio_pin_p, enum gpio_pin_irq_type irq_type);

void gpio_disable_pin_irq(const struct gpio_pin *gpio_pin_p);

void gpio_clear_pin_irq(const struct gpio_pin *gpio_pin_p);

#endif /* SOURCES_BUILDING_BLOCKS_GPIO_DRIVER_H_ */
