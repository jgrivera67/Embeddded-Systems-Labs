/**
 * @file color_led.c
 *
 * Multi-color LED services implementation
 *
 * @author: German Rivera
 */
#include "color_led.h"
#include "runtime_checks.h"
#include "runtime_log.h"
#include "gpio_driver.h"

enum rgb_led_pins {
    RED_LED_PIN = 0,
    GREEN_LED_PIN,
    BLUE_LED_PIN,
    NUM_RGB_LED_PINS
};

/**
 * FRDM board RGB LED pins
 */
static const struct gpio_pin g_rgb_led_pins[] = {
    [RED_LED_PIN] = GPIO_PIN_INITIALIZER(PIN_PORT_B,
    									 RGB_LED_RED_PIN_BIT_INDEX,
                                         PIN_FUNCTION_ALT1,
                                         false),

    [GREEN_LED_PIN] = GPIO_PIN_INITIALIZER(PIN_PORT_E,
    									   RGB_LED_GREEN_PIN_BIT_INDEX,
                                           PIN_FUNCTION_ALT1,
                                           false),

    [BLUE_LED_PIN] = GPIO_PIN_INITIALIZER(PIN_PORT_B,
    									  RGB_LED_BLUE_PIN_BIT_INDEX,
                                          PIN_FUNCTION_ALT1,
                                          false),
};

/**
 * State variables of a multi-color LED
 */
struct color_led {
    bool initialized;
    led_color_t current_color;
};

static struct color_led g_color_led = {
    .initialized = false,
	.current_color = LED_COLOR_BLACK,
};

/**
 * Initializes the multi-color LED
 *
 * NOTE: For now we leverage KSDK code for this. In the future,
 * we will write our own code for this.
 */
void color_led_init(void)
{
    D_ASSERT(!g_color_led.initialized);

    for (int i = 0; i < NUM_RGB_LED_PINS; i++) {
        gpio_configure_pin(&g_rgb_led_pins[i], 0, true);
        gpio_deactivate_output_pin(&g_rgb_led_pins[i]);
    }

    g_color_led.current_color = LED_COLOR_BLACK;
    g_color_led.initialized = true;
}


/**
 * Set the current color of the multi-color LED
 *
 * @param new_color new color to be set on the LED
 *
 * @return previous color the LED had
 */
led_color_t color_led_set(led_color_t new_color)
{
	uint32_t new_color_mask = new_color;
    led_color_t old_color = g_color_led.current_color;

    if (!g_color_led.initialized) {
        return old_color;
    }

    for (int i = 0; i < NUM_RGB_LED_PINS; i++) {
        if (g_rgb_led_pins[i].pin_bit_mask & new_color_mask) {
            gpio_activate_output_pin(&g_rgb_led_pins[i]);
        } else {
            gpio_deactivate_output_pin(&g_rgb_led_pins[i]);
        }
    }
    g_color_led.current_color = new_color;
    return old_color;
}


void color_led_toggle(led_color_t color)
{
	uint32_t color_mask = color;

    D_ASSERT(g_color_led.initialized);

    for (int i = 0; i < NUM_RGB_LED_PINS; i++) {
        if (g_rgb_led_pins[i].pin_bit_mask & color_mask) {
            gpio_toggle_output_pin(&g_rgb_led_pins[i]);
        }
    }

    g_color_led.current_color ^= color;
}
