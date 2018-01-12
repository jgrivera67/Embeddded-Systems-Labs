/**
 * @file uart_driver.h
 *
 * UART driver interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_UART_DRIVER_H_
#define SOURCES_BUILDING_BLOCKS_UART_DRIVER_H_

#include <stdint.h>
#include "runtime_checks.h"
#include "microcontroller.h"

/**
 *  Default UART transmission mode: 8-bits, no-parity, 1 stop bit
 */
#define UART_DEFAULT_MODE        UINT8_C(0)

/**
 * Const fields of a UART device (to be placed in flash)
 */
struct uart_device {
#   define UART_DEVICE_SIGNATURE  GEN_SIGNATURE('U', 'A', 'R', 'T')
    uint32_t urt_signature;
    struct uart_device_var *urt_var_p;
    USART_TypeDef *urt_mmio_uart_p;
    volatile uint32_t *urt_mmio_tx_port_pcr_p;
    volatile uint32_t *urt_mmio_rx_port_pcr_p;
    uint32_t urt_mmio_pin_mux_selector_mask;
    uint32_t urt_mmio_clock_gate_mask;
    uint32_t urt_source_clock_freq_in_hz;
    IRQn_Type urt_irq_num;
};


void uart_init(
        const struct uart_device *uart_device_p,
        uint32_t baud,
        uint8_t mode);

void uart_stop(const struct uart_device *uart_device_p);

void uart_putchar(
    const struct uart_device *uart_device_p,
    uint8_t c);

uint8_t uart_getchar(const struct uart_device *uart_device_p);

int uart_getchar_non_blocking(const struct uart_device *uart_device_p);

extern const struct uart_device g_uart_devices[];

#endif /* SOURCES_BUILDING_BLOCKS_UART_DRIVER_H_ */
