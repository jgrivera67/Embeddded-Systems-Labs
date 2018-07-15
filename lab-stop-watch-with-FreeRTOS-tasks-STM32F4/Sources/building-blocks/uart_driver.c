/**
 * @file uart_driver.c
 *
 * UART driver implementation
 *
 * @author German Rivera
 */
#include "uart_driver.h"
#include "atomic_utils.h"
#include "io_utils.h"
#include "runtime_checks.h"
#include "mem_utils.h"
#include "byte_ring_buffer.h"
#include "interrupt_vector_table.h"

/**
 * Serial communication parameters for the serial port used as the console
 * port
 */
#define UART_RECEIVE_QUEUE_SIZE_IN_BYTES    UINT16_C(16)

/**
 * Non-const fields of a UART device (to be placed in SRAM)
 */
struct uart_device_var {
    bool urt_initialized;
    uint32_t urt_received_bytes_dropped;
    uint32_t urt_errors;
    struct byte_ring_buffer urt_receive_queue;
    uint8_t urt_receive_queue_data[UART_RECEIVE_QUEUE_SIZE_IN_BYTES];
    uint8_t urt_tx_fifo_size;
    uint8_t urt_rx_fifo_size;
    bool urt_fifos_enabled;
};


/**
 * Global array of non-const structures for UART devices
 * (allocated in SRAM space)
 */
static struct uart_device_var g_uart_devices_var[] =
{
    [0] = {
        .urt_initialized = false,
        .urt_received_bytes_dropped = 0,
        .urt_errors = 0,
    },
};

/**
 * Global array of const structures for UART devices
 * (allocated in flash space)
 */
const struct uart_device g_uart_devices[] =
{
    [0] = {
        .urt_signature = UART_DEVICE_SIGNATURE,
        .urt_var_p = &g_uart_devices_var[0],
        .urt_mmio_regs_p = USART2,
        .urt_tx_pin = PIN_INITIALIZER(PIN_PORT_A, 2,
			              PIN_MODE_ALTERNATE_FUNCTION,
				      PIN_ALTERNATE_FUNCTION7),
        .urt_rx_pin = PIN_INITIALIZER(PIN_PORT_A, 3,
			              PIN_MODE_ALTERNATE_FUNCTION,
				      PIN_ALTERNATE_FUNCTION7),
        .urt_mmio_clock_gate_reg_p = &RCC->APB1ENR,
        .urt_mmio_clock_gate_mask = RCC_APB1ENR_USART2EN,
	.urt_source_clock_freq_in_hz = APB1_CLOCK_FREQ_IN_HZ,
        .urt_irq_num = USART2_IRQn,
    },
};

C_ASSERT(
    ARRAY_SIZE(g_uart_devices) >= ARRAY_SIZE(g_uart_devices_var));


static void uart_set_baud_rate(
    const struct uart_device *uart_device_p,
    uint32_t baud_rate)
{
    uint32_t reg_value;
    uint8_t usartdiv_int_part;
    uint16_t usartdiv_fraction_part;
    USART_TypeDef *const uart_mmio_registers_p = uart_device_p->urt_mmio_regs_p;

    D_ASSERT(baud_rate == 115200);

    /*
     * Calculate values of BRR fields assuming oversampling of 16 (OVER8 == 0):
     * (See section 30.4.4, table 190 of STM32F4xx reference manual)
     */
    if (uart_device_p->urt_source_clock_freq_in_hz == UINT32_C(42000000)) {
	usartdiv_int_part = 22;
	usartdiv_fraction_part = 8125;
    } else if (uart_device_p->urt_source_clock_freq_in_hz == UINT32_C(84000000)) {
	usartdiv_int_part = 45;
	usartdiv_fraction_part = 5625;
    } else {
	D_ASSERT(false);
    }

    D_ASSERT(usartdiv_fraction_part > 999 && usartdiv_fraction_part <= 9999);
    uint16_t encoded_int_part = usartdiv_int_part;
    uint8_t encoded_fraction_part = (16 * (uint32_t)usartdiv_fraction_part) / 10000;
    if ((16 * (uint32_t)usartdiv_fraction_part) % 10000 > 5000) {
        encoded_fraction_part++;
    }

    if (encoded_fraction_part > 0xf) {
	encoded_int_part += (encoded_fraction_part >> 4);
	encoded_fraction_part &= 0xf;
    }

    D_ASSERT(encoded_int_part < BIT(12));
    D_ASSERT(encoded_fraction_part < BIT(4));

    reg_value = 0;
    SET_BIT_FIELD(reg_value, USART_BRR_DIV_Mantissa_Msk,
	          USART_BRR_DIV_Mantissa_Pos, encoded_int_part);
    SET_BIT_FIELD(reg_value, USART_BRR_DIV_Fraction_Msk,
	          USART_BRR_DIV_Fraction_Pos, encoded_fraction_part);
    WRITE_MMIO_REGISTER(&uart_mmio_registers_p->BRR, reg_value);
}


void uart_init(
    const struct uart_device *uart_device_p,
    uint32_t baud_rate,
    uint8_t mode)
{
    uint32_t reg_value;

    D_ASSERT(
        uart_device_p->urt_signature == UART_DEVICE_SIGNATURE);

    struct uart_device_var *uart_var_p = uart_device_p->urt_var_p;
    USART_TypeDef *uart_mmio_registers_p = uart_device_p->urt_mmio_regs_p;

    D_ASSERT(uart_var_p != NULL);
    D_ASSERT(!uart_var_p->urt_initialized);

    /*
     * Only the default transmission mode is supported for now
     * (8-bit mode, no parity, 1 stop bit)
     */
    D_ASSERT(mode == UART_DEFAULT_MODE);

    /*
     * Enable clock for the UART:
     */
    reg_value = READ_MMIO_REGISTER(uart_device_p->urt_mmio_clock_gate_reg_p);
    reg_value |= uart_device_p->urt_mmio_clock_gate_mask;
    WRITE_MMIO_REGISTER(uart_device_p->urt_mmio_clock_gate_reg_p, reg_value);

    /*
     * Disable UART's transmitter and receiver, while UART is being
     * configured:
     */
    reg_value = uart_mmio_registers_p->CR1;
    reg_value &= ~USART_CR1_UE;
    uart_mmio_registers_p->CR1 = reg_value;

    reg_value &= ~USART_CR1_OVER8;  /* oversampling by 16 */
    reg_value &= ~USART_CR1_M;	    /* 8 data bits */
    reg_value &= ~(USART_CR1_PCE | USART_CR1_PS);   /* parity none */
    reg_value |= (USART_CR1_TE | USART_CR1_RE); /* Tx/Rx direction */
    uart_mmio_registers_p->CR1 = reg_value;

    reg_value = uart_mmio_registers_p->CR2;
    reg_value &= ~USART_CR2_STOP; /* 1 stop bit */
    reg_value &= ~USART_CR2_CLKEN; /* SCLK pin disabled */
    uart_mmio_registers_p->CR2 = reg_value;

    reg_value = uart_mmio_registers_p->CR3;
    reg_value &= ~(USART_CR3_RTSE | USART_CR3_CTSE); /* hw flow control off */
    uart_mmio_registers_p->CR3 = reg_value;

    /*
     * Configure Tx and Rx pins:
     */
    set_pin_function(&uart_device_p->urt_tx_pin, 0x0);
    set_pin_function(&uart_device_p->urt_rx_pin, 0x0);

    /*
    * Calculate baud rate settings:
    */
    uart_set_baud_rate(uart_device_p, baud_rate);

    /*
     * Enable UART's transmitter and receiver:
     */
    reg_value = uart_mmio_registers_p->CR1;
    reg_value |= USART_CR1_UE;
    uart_mmio_registers_p->CR1 = reg_value;

    uart_var_p->urt_initialized = true;
}


/**
 * Send a character over a UART serial port, doing polling until the
 * character gets transmitted.
 */
void uart_putchar(const struct uart_device *uart_device_p, uint8_t c)
{
    struct uart_device_var *const uart_var_p = uart_device_p->urt_var_p;
    USART_TypeDef *uart_mmio_registers_p = uart_device_p->urt_mmio_regs_p;
    uint32_t reg_value;

    if (!uart_var_p->urt_initialized) {
        return;
    }

    /*
     * Do polling until the UART's transmit buffer is empty:
     */
    for ( ; ; ) {
        reg_value = READ_MMIO_REGISTER(&uart_mmio_registers_p->SR);
        if ((reg_value & USART_SR_TXE) != 0) {
            break;
        }
    }

    WRITE_MMIO_REGISTER(&uart_mmio_registers_p->DR, c);
}


/**
 * Reads the next character received on the console UART, if any.
 *
 * If no character has been received, it returns right away with -1.
 * at the UART.
 *
 * NOTE: This function is to be used only if interrupts are disabled.
 *
 * @return: ASCII code of the character received, on success
 * @return: -1, if no character was available to read from the UART.
 */
int uart_getchar_non_blocking(const struct uart_device *uart_device_p)
{
    uint32_t reg_value;
    struct uart_device_var *const uart_var_p = uart_device_p->urt_var_p;
    USART_TypeDef *uart_mmio_registers_p = uart_device_p->urt_mmio_regs_p;

    D_ASSERT(uart_var_p->urt_initialized);
    D_ASSERT(CPU_INTERRUPTS_ARE_DISABLED());
    reg_value = READ_MMIO_REGISTER(&uart_mmio_registers_p->SR);
    if (reg_value & USART_SR_RXNE) {
        /*
         * Read next byte received:
         */
         reg_value = READ_MMIO_REGISTER(&uart_mmio_registers_p->DR);
         return (int)reg_value;
    } else {
        return -1;
    }
}
