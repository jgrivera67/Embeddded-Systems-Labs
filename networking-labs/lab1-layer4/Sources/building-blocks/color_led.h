/**
 * @file color_led.h
 *
 * Multi-color LED services interface
 *
 * @author: German Rivera
 */
#ifndef SOURCES_COLOR_LED_H_
#define SOURCES_COLOR_LED_H_

#include "io_utils.h"

#define RGB_LED_RED_PIN_BIT_INDEX      22	/* in port B */
#define RGB_LED_GREEN_PIN_BIT_INDEX    26	/* in port E */
#define RGB_LED_BLUE_PIN_BIT_INDEX     21	/* in port B */

#define RGB_LED_RED_PIN_MASK    BIT(RGB_LED_RED_PIN_BIT_INDEX)
#define RGB_LED_GREEN_PIN_MASK  BIT(RGB_LED_GREEN_PIN_BIT_INDEX)
#define RGB_LED_BLUE_PIN_MASK   BIT(RGB_LED_BLUE_PIN_BIT_INDEX)

enum led_colors {
    LED_COLOR_BLACK = 0x0,
    LED_COLOR_RED = RGB_LED_RED_PIN_MASK,
    LED_COLOR_GREEN = RGB_LED_GREEN_PIN_MASK,
    LED_COLOR_YELLOW = (RGB_LED_RED_PIN_MASK | RGB_LED_GREEN_PIN_MASK),
    LED_COLOR_BLUE = RGB_LED_BLUE_PIN_MASK,
    LED_COLOR_MAGENTA = (RGB_LED_RED_PIN_MASK | RGB_LED_BLUE_PIN_MASK),
    LED_COLOR_CYAN = (RGB_LED_GREEN_PIN_MASK | RGB_LED_BLUE_PIN_MASK),
    LED_COLOR_WHITE = (RGB_LED_RED_PIN_MASK | RGB_LED_GREEN_PIN_MASK | RGB_LED_BLUE_PIN_MASK)
};

typedef enum led_colors led_color_t;

void color_led_init(void);

led_color_t color_led_set(led_color_t new_color);

void color_led_toggle(led_color_t color);

#endif /* SOURCES_COLOR_LED_H_ */
