/**
 * @file pin_config.h
 *
 * Pin configurator interface
 *
 * @author: German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_PIN_CONFIG_H_
#define SOURCES_BUILDING_BLOCKS_PIN_CONFIG_H_

#include <stdint.h>
#include "microcontroller.h"

#define NUM_PINS_PER_PORT   16

/**
 * Initializer for a pin_info structure
 */
#define PIN_INITIALIZER(_pin_port, _pin_index, _pin_mode, _pin_function)       \
        {                                                                      \
            .pin_port = (_pin_port),                                           \
            .pin_index = (_pin_index),                                         \
            .pin_mode = (_pin_mode),                                           \
            .pin_alternate_function = (_pin_function),                         \
        }

/**
 * Pin bit index (0 .. NUM_PINS_PER_PORT)
 */
typedef uint8_t pin_index_t;

/**
 * Pin ports
 */
enum pin_ports {
    PIN_PORT_A = 0,
    PIN_PORT_B,
    PIN_PORT_C,
    PIN_PORT_D,
    PIN_PORT_E,
    PIN_PORT_H,
    NUM_PIN_PORTS
};

typedef enum pin_ports pin_port_t;

enum pin_modes {
    PIN_MODE_GPIO_INPUT = 0,
    PIN_MODE_GPIO_OUTPUT,
    PIN_MODE_ALTERNATE_FUNCTION,
    PIN_MODE_ANALOG,
};

typedef enum pin_modes pin_mode_t;

enum pin_alternate_functions {
    PIN_ALTERNATE_FUNCTION0 = 0,
    PIN_ALTERNATE_FUNCTION1,
    PIN_ALTERNATE_FUNCTION2,
    PIN_ALTERNATE_FUNCTION3,
    PIN_ALTERNATE_FUNCTION4,
    PIN_ALTERNATE_FUNCTION5,
    PIN_ALTERNATE_FUNCTION6,
    PIN_ALTERNATE_FUNCTION7,
    PIN_ALTERNATE_FUNCTION8,
    PIN_ALTERNATE_FUNCTION9,
    PIN_ALTERNATE_FUNCTION10,
    PIN_ALTERNATE_FUNCTION11,
    PIN_ALTERNATE_FUNCTION12,
    PIN_ALTERNATE_FUNCTION13,
    PIN_ALTERNATE_FUNCTION14,
    PIN_ALTERNATE_FUNCTION15,

    PIN_ALTERNATE_FUNCTION_NONE
};

typedef enum pin_alternate_functions pin_alternate_function_t;

/**
 * Pin configuration parameters
 */
struct pin_info {
    pin_port_t pin_port;
    pin_index_t pin_index;
    pin_mode_t pin_mode;
    pin_alternate_function_t pin_alternate_function;
};

void pin_config_init(void);

void set_pin_function(const struct pin_info *pin_p, uint32_t pin_flags);

extern GPIO_TypeDef *const g_pin_port_regs[NUM_PIN_PORTS];

#endif /* SOURCES_BUILDING_BLOCKS_PIN_CONFIG_H_ */
