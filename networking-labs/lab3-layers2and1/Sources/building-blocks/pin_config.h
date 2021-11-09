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
#include <MK64F12.h>

#define NUM_PINS_PER_PORT   32

/**
 * Initializer for a pin_info structure
 */
#define PIN_INITIALIZER(_pin_port, _pin_index, _pin_function)                  \
        {                                                                      \
            .pin_port = (_pin_port),                                           \
            .pin_index = (_pin_index),                                         \
            .pin_function = (_pin_function),                                   \
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
    NUM_PIN_PORTS
};

typedef enum pin_ports pin_port_t;

enum pin_functions {
    PIN_FUNCTION_ALT0 = 0,
    PIN_FUNCTION_ALT1,
    PIN_FUNCTION_ALT2,
    PIN_FUNCTION_ALT3,
    PIN_FUNCTION_ALT4,
    PIN_FUNCTION_ALT5,
    PIN_FUNCTION_ALT6,
    PIN_FUNCTION_ALT7,
};

typedef enum pin_functions pin_function_t;

/**
 * Pin configuration parameters
 */
struct pin_info {
    pin_port_t pin_port;
    pin_index_t pin_index;
    pin_function_t pin_function;
};

void pin_config_init(void);

void set_pin_function(const struct pin_info *pin_p, uint32_t pin_flags);

extern PORT_Type *const g_pin_port_regs[NUM_PIN_PORTS];

#endif /* SOURCES_BUILDING_BLOCKS_PIN_CONFIG_H_ */
