/**
 * @file KL25Z_interrupt_vector_table.c
 *
 * KL25Z interrupt vector table implementation
 *
 * @author: German Rivera
 */
#include "interrupt_vector_table.h"
#include "cortex_m_startup.h"
#include "arm_cmsis.h"
#include "compile_time_checks.h"
#include "runtime_checks.h"
#include "mem_utils.h"

#include <stdint.h>
#include <stdbool.h>

/**
 * Default Non Maskable Interrupt exception handler
 */
void NMI_Handler(void)
{
	D_ASSERT(false);

	/*
	 * Trap the processor in a dummy infinite loop
	 */
	for ( ; ; )
		;
}

/**
 * Default BusFault exception handler
 */
void BusFault_Handler(void)
{
	D_ASSERT(false);

	/*
	 * Trap the processor in a dummy infinite loop
	 */
	for ( ; ; )
		;
}

/**
 * Default UsageFault exception handler
 */
void UsageFault_Handler(void)
{
	D_ASSERT(false);

	/*
	 * Trap the processor in a dummy infinite loop
	 */
	for ( ; ; )
		;
}

/**
 * Default DebugMonitor exception handler
 */
void DebugMonitor_Handler(void)
{
	D_ASSERT(false);

	/*
	 * Trap the processor in a dummy infinite loop
	 */
	for ( ; ; )
		;
}

#if 0 /* FreeRTOS defines its own handlers for SVC, PenSV and SysTick */
/**
 * Default SVC (supervisor call) exception handler
 */
void SVC_Handler(void)
{
	D_ASSERT(false);

	/*
	 * Trap the processor in a dummy infinite loop
	 */
	for ( ; ; )
		;
}


/**
 * Default PendSV exception handler
 */
void PendSV_Handler(void)
{
	D_ASSERT(false);

	/*
	 * Trap the processor in a dummy infinite loop
	 */
	for ( ; ; )
		;
}


/**
 * Default SysTick exception handler
 */
void SysTick_Handler(void)
{
	D_ASSERT(false);

	/*
	 * Trap the processor in a dummy infinite loop
	 */
	for ( ; ; )
		;
}
#endif


/**
 * Default interrupt handler for all IRQs
 */
static void unexpected_irq_handler(void)
{
	D_ASSERT(false);

	/*
	 * Trap the processor in a dummy infinite loop
	 */
	for ( ; ; )
		;
}


/**
 * Interrupt Vector Table
 */
isr_function_t *const g_interrupt_vector_table[] __attribute__ ((section(".isr_vector"))) = {
    [0] = (void *)__stack_end__,

    /*
     * Processor exceptions
     */
    [1] = Reset_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(NonMaskableInt_IRQn)] = NMI_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(MemoryManagement_IRQn)] = MemoryManagement_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(BusFault_IRQn)] = BusFault_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(UsageFault_IRQn)] = UsageFault_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SVCall_IRQn)] = SVC_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DebugMonitor_IRQn)] = DebugMonitor_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PendSV_IRQn)] = PendSV_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SysTick_IRQn)] = SysTick_Handler,

    /*
     * Interrupts external to the Cortex-M core
     */
    [IRQ_NUMBER_TO_VECTOR_NUMBER(WWDG_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PVD_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(TAMP_STAMP_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(RTC_WKUP_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(FLASH_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(RCC_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(EXTI0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(EXTI1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(EXTI2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(EXTI3_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(EXTI4_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA1_Stream0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA1_Stream1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA1_Stream2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA1_Stream3_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA1_Stream4_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA1_Stream5_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA1_Stream6_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(ADC_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(EXTI9_5_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(TIM1_BRK_TIM9_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(TIM1_UP_TIM10_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(TIM1_TRG_COM_TIM11_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(TIM1_CC_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(TIM2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(TIM3_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(TIM4_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(I2C1_EV_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(I2C1_ER_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(I2C2_EV_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(I2C2_ER_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SPI1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SPI2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(USART1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(USART2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(EXTI15_10_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(RTC_Alarm_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(OTG_FS_WKUP_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA1_Stream7_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SDIO_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(TIM5_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SPI3_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA2_Stream0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA2_Stream1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA2_Stream2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA2_Stream3_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA2_Stream4_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(OTG_FS_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA2_Stream5_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA2_Stream6_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA2_Stream7_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(USART6_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(I2C3_EV_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(I2C3_ER_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(FPU_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SPI4_IRQn)] = unexpected_irq_handler,
};
C_ASSERT(ARRAY_SIZE(g_interrupt_vector_table) ==
	 IRQ_NUMBER_TO_VECTOR_NUMBER(SPI4_IRQn) + 1);

