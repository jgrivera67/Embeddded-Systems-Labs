/**
 * @file interrupt_vector_table.h
 *
 * interrupt vector table interface
 *
 * @author: German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_INTERRUPT_VECTOR_TABLE_H_
#define SOURCES_BUILDING_BLOCKS_INTERRUPT_VECTOR_TABLE_H_

#include "microcontroller.h"

#define ADC_INTERRUPT_PRIORITY                  (MCU_HIGHEST_INTERRUPT_PRIORITY)

#define I2C_INTERRUPT_PRIORITY                  (MCU_HIGHEST_INTERRUPT_PRIORITY)

#define DMA_INTERRUPT_PRIORITY                  (MCU_HIGHEST_INTERRUPT_PRIORITY)

#define HW_TIMER_INTERRUPT_PRIORITY             (MCU_HIGHEST_INTERRUPT_PRIORITY + 1)

#define HW_PERIODIC_TIMER_INTERRUPT_PRIORITY    (MCU_HIGHEST_INTERRUPT_PRIORITY + 1)

#define ACCELEROMETER_INTERRUPT_PRIORITY		(MCU_HIGHEST_INTERRUPT_PRIORITY + 2)

#define UART_INTERRUPT_PRIORITY                 (MCU_LOWEST_INTERRUPT_PRIORITY)

/**
 * Base interrupt vector number for external IRQs
 */
#define CORTEX_M_IRQ_VECTOR_BASE     16

/**
 * Convert an IRQ number to an interrupt vector number (vector table index)
 * IRQ number can be negative (for internal interrupts)
 */
#define IRQ_NUMBER_TO_VECTOR_NUMBER(_irq_number) \
        (((IRQn_Type)(_irq_number)) + CORTEX_M_IRQ_VECTOR_BASE)


/**
 * Interrupt service routine (ISR) function type
 */
typedef void isr_function_t(void);

/*
 * Exception handlers:
 */

void Reset_Handler(void);

void NMI_Handler(void);

void BusFault_Handler(void);

void UsageFault_Handler(void);

void DebugMonitor_Handler(void);

void MemoryManagement_Handler(void);

void SVC_Handler(void);

void PendSV_Handler(void);

void SysTick_Handler(void);

/*
 * Interrupt handlers:
 */

void lptmr0_irq_handler(void);

extern isr_function_t *const g_interrupt_vector_table[];

#endif /* SOURCES_BUILDING_BLOCKS_INTERRUPT_VECTOR_TABLE_H_ */
