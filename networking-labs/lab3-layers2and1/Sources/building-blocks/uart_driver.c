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

    [4] = {
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
        .urt_mmio_regs_p = UART0_BASE_PTR,
        .urt_tx_pin = PIN_INITIALIZER(PIN_PORT_B, 17, PIN_FUNCTION_ALT3),
        .urt_rx_pin = PIN_INITIALIZER(PIN_PORT_B, 16, PIN_FUNCTION_ALT3),
		.urt_rx_pin_pullup_resistor_enabled = false,
        .urt_mmio_clock_gate_reg_p = &SIM_SCGC4,
        .urt_mmio_clock_gate_mask = SIM_SCGC4_UART0_MASK,
	    .urt_source_clock_freq_in_hz = MCU_SYSTEM_CLOCK_FREQ_IN_HZ, /* see table 5-2 */
        .urt_rx_tx_irq_num = UART0_RX_TX_IRQn,
        .urt_error_irq_num = UART0_ERR_IRQn,
    },

	[4] = {
        .urt_signature = UART_DEVICE_SIGNATURE,
        .urt_var_p = &g_uart_devices_var[4],
        .urt_mmio_regs_p = UART4_BASE_PTR,
        .urt_tx_pin = PIN_INITIALIZER(PIN_PORT_C, 15, PIN_FUNCTION_ALT3),
        .urt_rx_pin = PIN_INITIALIZER(PIN_PORT_C, 14, PIN_FUNCTION_ALT3),
        .urt_rx_pin_pullup_resistor_enabled = true,
        .urt_mmio_clock_gate_reg_p = &SIM_SCGC1,
        .urt_mmio_clock_gate_mask = SIM_SCGC1_UART4_MASK,
        .urt_source_clock_freq_in_hz = MCU_BUS_CLOCK_FREQ_IN_HZ, /* see table 5-2 */
        .urt_rx_tx_irq_num = UART4_RX_TX_IRQn,
        .urt_error_irq_num = UART4_ERR_IRQn,
    },
};

C_ASSERT(
    ARRAY_SIZE(g_uart_devices) >= ARRAY_SIZE(g_uart_devices_var));


static void uart_set_baud_rate(
    const struct uart_device *uart_device_p,
    uint32_t baud_rate)
{
	uint32_t reg_value;
    UART_Type *uart_mmio_registers_p = uart_device_p->urt_mmio_regs_p;

   /*
    * Calculate baud rate settings:
    */
    uint32_t uart_clk = uart_device_p->urt_source_clock_freq_in_hz;
    uint16_t sbr_val = uart_clk / (baud_rate * 16);

    D_ASSERT(sbr_val >= 1 && sbr_val <= 0x1FFF);

     /*
      * Set SBR high-part field in the UART's BDH register:
      */
    reg_value = READ_MMIO_REGISTER(&uart_mmio_registers_p->BDH);
    SET_BIT_FIELD(reg_value, UART_BDH_SBR_MASK, UART_BDH_SBR_SHIFT,
                  (sbr_val & 0x1F00) >> 8);
    WRITE_MMIO_REGISTER(&uart_mmio_registers_p->BDH, reg_value);

    /*
     * Set SBR low byte in the UART's BDL register:
     */
    WRITE_MMIO_REGISTER(&uart_mmio_registers_p->BDL,
                        sbr_val & UART_BDL_SBR_MASK);
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
    UART_Type *uart_mmio_registers_p = uart_device_p->urt_mmio_regs_p;

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
    reg_value = READ_MMIO_REGISTER(&UART_C2_REG(uart_mmio_registers_p));
    reg_value &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK);
    WRITE_MMIO_REGISTER(&UART_C2_REG(uart_mmio_registers_p), reg_value);

    /*
     * Configure the uart transmission mode:
     */
    WRITE_MMIO_REGISTER(
        &UART_C1_REG(uart_mmio_registers_p), mode);

    /*
     * Get Tx FIFO size
     */
    reg_value = READ_MMIO_REGISTER(&UART_PFIFO_REG(uart_mmio_registers_p));
    uint8_t fifo_size_field = GET_BIT_FIELD(reg_value,
					                        UART_PFIFO_TXFIFOSIZE_MASK,
					                        UART_PFIFO_TXFIFOSIZE_SHIFT);
    if (fifo_size_field == 0x0) {
        uart_var_p->urt_tx_fifo_size = 1;
    } else {
        uart_var_p->urt_tx_fifo_size = 1 << (fifo_size_field + 1);
    }

    /*
     * Get Rx FIFO size:
     */
    fifo_size_field = GET_BIT_FIELD(reg_value,
				                    UART_PFIFO_RXFIFOSIZE_MASK,
				                    UART_PFIFO_RXFIFOSIZE_SHIFT);
    if (fifo_size_field == 0x0) {
        uart_var_p->urt_rx_fifo_size = 1;
    } else {
        uart_var_p->urt_rx_fifo_size = 1 << (fifo_size_field + 1);
    }

    /*
     * Configure Tx and RX FIFOs:
     * - Rx FIFO water mark = 1 (generate interrupt when Rx FIFO is not empty)
     * - Enable Tx and Rx FIFOs
     * - Flush Tx and Rx FIFOs
     */
    WRITE_MMIO_REGISTER(&uart_mmio_registers_p->RWFIFO, 1);
    WRITE_MMIO_REGISTER(&uart_mmio_registers_p->PFIFO,
    		            UART_PFIFO_TXFE_MASK | UART_PFIFO_RXFE_MASK);
    WRITE_MMIO_REGISTER(&uart_mmio_registers_p->CFIFO,
	                    UART_CFIFO_TXFLUSH_MASK | UART_CFIFO_RXFLUSH_MASK);

    uart_var_p->urt_fifos_enabled = true;

    /*
     * Configure Tx and Rx pins:
     */
    set_pin_function(&uart_device_p->urt_tx_pin, PORT_PCR_DSE_MASK);
    if (uart_device_p->urt_rx_pin_pullup_resistor_enabled) {
        set_pin_function(&uart_device_p->urt_rx_pin, PORT_PCR_DSE_MASK|PORT_PCR_PS_MASK|PORT_PCR_PE_MASK);
    } else {
        set_pin_function(&uart_device_p->urt_rx_pin, PORT_PCR_DSE_MASK);
    }

    /*
    * Calculate baud rate settings:
    */
    uart_set_baud_rate(uart_device_p, baud_rate);

    /*
     * Initialize receive queue:
     */
    byte_ring_buffer_init(&uart_var_p->urt_receive_queue,
                          uart_var_p->urt_receive_queue_data,
                          sizeof uart_var_p->urt_receive_queue_data);

    /*
     * Enable generation of Rx interrupts and disable generation of
     * Tx interrupts:
     */
    reg_value = READ_MMIO_REGISTER(&uart_mmio_registers_p->C2);
    reg_value |= UART_C2_RIE_MASK;
    reg_value &= ~UART_C2_TIE_MASK;
    WRITE_MMIO_REGISTER(&uart_mmio_registers_p->C2, reg_value);

    /*
     * Enable interrupts in the interrupt controller (NVIC):
     */
    nvic_setup_irq(uart_device_p->urt_rx_tx_irq_num, UART_INTERRUPT_PRIORITY);
    nvic_setup_irq(uart_device_p->urt_error_irq_num, UART_INTERRUPT_PRIORITY);

    /*
     * Enable UART's transmitter and receiver:
     */
    reg_value = READ_MMIO_REGISTER(&uart_mmio_registers_p->C2);
    reg_value |= (UART_C2_TE_MASK | UART_C2_RE_MASK);
    WRITE_MMIO_REGISTER(&uart_mmio_registers_p->C2, reg_value);

    uart_var_p->urt_initialized = true;
}


void uart_stop(
    const struct uart_device *uart_device_p)
{
    uint32_t reg_value;
    struct uart_device_var *uart_var_p = uart_device_p->urt_var_p;
    UART_Type *uart_mmio_registers_p = uart_device_p->urt_mmio_regs_p;

    D_ASSERT(uart_var_p != NULL);
    D_ASSERT(uart_var_p->urt_initialized);

    /*
     * Disable UART's transmitter and receiver:
     */
    reg_value = READ_MMIO_REGISTER(&uart_mmio_registers_p->C2);
    reg_value &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK);
    WRITE_MMIO_REGISTER(&uart_mmio_registers_p->C2, reg_value);

    /*
     * Disable clock for the UART:
     */
    reg_value = READ_MMIO_REGISTER(uart_device_p->urt_mmio_clock_gate_reg_p);
    reg_value &= ~uart_device_p->urt_mmio_clock_gate_mask;
    WRITE_MMIO_REGISTER(uart_device_p->urt_mmio_clock_gate_reg_p, reg_value);

    uart_var_p->urt_initialized = false;
}


static void uart_rx_tx_irq_handler(
    const struct uart_device *uart_device_p)
{
    uint32_t reg_value;
    uint_fast8_t i;
    bool entry_written;
    struct uart_device_var *uart_var_p = uart_device_p->urt_var_p;
    UART_Type *uart_mmio_registers_p = uart_device_p->urt_mmio_regs_p;

    D_ASSERT(uart_var_p != NULL);
    D_ASSERT(uart_var_p->urt_initialized);

    reg_value = READ_MMIO_REGISTER(&uart_mmio_registers_p->S1);

    /*
     * This interrupt was triggered by "Receive data register full":
     */
    D_ASSERT(reg_value & UART_S1_RDRF_MASK);

    uint_fast8_t rx_fifo_length =
		READ_MMIO_REGISTER(&uart_mmio_registers_p->RCFIFO);

    do {
        D_ASSERT(rx_fifo_length > 0 &&
        		 rx_fifo_length <= uart_var_p->urt_rx_fifo_size);

        /*
	     * Drain the Rx FIFO but leave one byte in it:
	     */
	    for (i = 0; i < rx_fifo_length - 1; i++) {
            reg_value = READ_MMIO_REGISTER(&uart_mmio_registers_p->D);
            entry_written = byte_ring_buffer_write_non_blocking(&uart_var_p->urt_receive_queue,
            		                                            reg_value);
            if (!entry_written) {
                uart_var_p->urt_received_bytes_dropped ++;
                break;
            }
        }

        /*
         * Drain the last byte from the FIFO in a special way to clear the
         * RDRF flag in the S1 register:
         * - read first the S1 register
         * - then read the D register
         */
        if (i == rx_fifo_length - 1) {
            (void)READ_MMIO_REGISTER(&uart_mmio_registers_p->S1);

            reg_value = READ_MMIO_REGISTER(&uart_mmio_registers_p->D);
            entry_written = byte_ring_buffer_write_non_blocking(&uart_var_p->urt_receive_queue,
            		                                            reg_value);
            if (!entry_written) {
                uart_var_p->urt_received_bytes_dropped ++;
            }
        }

        rx_fifo_length = READ_MMIO_REGISTER(&uart_mmio_registers_p->RCFIFO);
    } while (rx_fifo_length != 0);
}


static void uart_error_irq_handler(
    const struct uart_device *uart_device_p)
{
    uint32_t reg_value;
    struct uart_device_var *uart_var_p = uart_device_p->urt_var_p;
    UART_Type *uart_mmio_registers_p = uart_device_p->urt_mmio_regs_p;

    reg_value = READ_MMIO_REGISTER(&uart_mmio_registers_p->S1);

    if (reg_value & (UART_S1_OR_MASK | UART_S1_NF_MASK | UART_S1_FE_MASK | UART_S1_PF_MASK)) {
        uart_var_p->urt_errors ++;
    }
}


/**
 * ISR for the UART0's Rx/Tx interrupt
 */
void uart0_rx_tx_irq_handler(void)
{
    D_ASSERT(CPU_INTERRUPTS_ARE_ENABLED());

    rtos_enter_isr();
    uart_rx_tx_irq_handler(&g_uart_devices[0]);
    rtos_exit_isr();
}


/**
 * ISR for the UART0's error interrupt
 */
void uart0_error_irq_handler(void)
{
    D_ASSERT(CPU_INTERRUPTS_ARE_ENABLED());

    rtos_enter_isr();
    uart_error_irq_handler(&g_uart_devices[0]);
    rtos_exit_isr();
}


/**
 * ISR for the UART0's Rx/Tx interrupt
 */
void uart4_rx_tx_irq_handler(void)
{
    D_ASSERT(CPU_INTERRUPTS_ARE_ENABLED());

    rtos_enter_isr();
    uart_rx_tx_irq_handler(&g_uart_devices[4]);
    rtos_exit_isr();
}


/**
 * ISR for the UART4's error interrupt
 */
void uart4_error_irq_handler(void)
{
    D_ASSERT(CPU_INTERRUPTS_ARE_ENABLED());

    rtos_enter_isr();
    uart_error_irq_handler(&g_uart_devices[4]);
    rtos_exit_isr();
}


/**
 * Send a character over a UART serial port, doing polling until the
 * character gets transmitted.
 */
void uart_putchar(const struct uart_device *uart_device_p, uint8_t c)
{
    struct uart_device_var *const uart_var_p = uart_device_p->urt_var_p;
    UART_Type *uart_mmio_registers_p = uart_device_p->urt_mmio_regs_p;
    uint32_t reg_value;

    if (!uart_var_p->urt_initialized) {
        return;
    }

    /*
     * Do polling until the UART's transmit buffer is empty:
     */
    for ( ; ; ) {
        reg_value = READ_MMIO_REGISTER(&uart_mmio_registers_p->S1);
        if ((reg_value & UART_S1_TDRE_MASK) != 0) {
            break;
        }
    }

    WRITE_MMIO_REGISTER(&uart_mmio_registers_p->D, c);
}


/**
 * Receive a character from a UART serial port, blocking the caller
 * if there are no characters to read
 */
uint8_t uart_getchar(const struct uart_device *uart_device_p)
{
    struct uart_device_var *const uart_var_p = uart_device_p->urt_var_p;
    uint8_t c;

    D_ASSERT(uart_var_p->urt_initialized);
    D_ASSERT(CPU_INTERRUPTS_ARE_ENABLED());
    c = byte_ring_buffer_read(&uart_var_p->urt_receive_queue);
    return c;
}


/**
 * Reads the next character received from a UART serial port, doing polling
 * until the character is received.
 *
 * NOTE: This function is to be used only when interrupts are disabled.
 */
uint8_t uart_getchar_with_polling(const struct uart_device *uart_device_p)
{
    uint32_t reg_value;
    struct uart_device_var *const uart_var_p = uart_device_p->urt_var_p;
    UART_Type *uart_mmio_registers_p = uart_device_p->urt_mmio_regs_p;

    D_ASSERT(uart_var_p->urt_initialized);
    D_ASSERT(CPU_INTERRUPTS_ARE_DISABLED());

    /*
     * Disable generation of receive interrupts:
     */
    reg_value = read_8bit_mmio_register(&UART_C2_REG(uart_mmio_registers_p));
    reg_value &= ~UART_C2_RIE_MASK;
    write_8bit_mmio_register(&UART_C2_REG(uart_mmio_registers_p), reg_value);

    /*
     * Do polling until the UART's receive buffer is not empty:
     */
    for ( ; ; ) {
        reg_value = READ_MMIO_REGISTER(&uart_mmio_registers_p->S1);
        if ((reg_value & UART_S1_RDRF_MASK) != 0) {
            break;
        }
    }

    uint8_t byte_received = READ_MMIO_REGISTER(&uart_mmio_registers_p->D);

    /*
     * Re-enable generation of receive interrupts:
     */
    reg_value = read_8bit_mmio_register(&UART_C2_REG(uart_mmio_registers_p));
    reg_value |= UART_C2_RIE_MASK;
    write_8bit_mmio_register(&UART_C2_REG(uart_mmio_registers_p), reg_value);

    return byte_received;
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
    UART_Type *uart_mmio_registers_p = uart_device_p->urt_mmio_regs_p;

    D_ASSERT(uart_var_p->urt_initialized);
    D_ASSERT(CPU_INTERRUPTS_ARE_DISABLED());
    reg_value = READ_MMIO_REGISTER(&uart_mmio_registers_p->S1);
    if (reg_value & UART_S1_RDRF_MASK) {
        /*
         * Read next byte received:
         */
         reg_value = READ_MMIO_REGISTER(&uart_mmio_registers_p->D);
         return (int)reg_value;
    } else {
        return -1;
    }
}
