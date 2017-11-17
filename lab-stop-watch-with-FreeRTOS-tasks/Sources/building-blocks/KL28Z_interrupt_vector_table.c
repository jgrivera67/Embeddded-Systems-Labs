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
#include "runtime_checks.h"
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


#if 0 /* FreeRTOS defines tis own handlers for SVC, PenSV and SysTick */
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
    [0] = (void *)__StackTop,

    /*
     * Processor exceptions
     */
    [1] = Reset_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(NonMaskableInt_IRQn)] = NMI_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(HardFault_IRQn)] = HardFault_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SVCall_IRQn)] = SVC_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PendSV_IRQn)] = PendSV_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SysTick_IRQn)] = SysTick_Handler,

    /*
     * Interrupts external to the Cortex-M core
     */
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA0_04_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA0_15_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA0_26_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA0_37_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(CTI0_DMA0_Error_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(FLEXIO0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(TPM0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(TPM1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(TPM2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(LPIT0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(LPSPI0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(LPSPI1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(LPUART0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(LPUART1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(LPI2C0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(LPI2C1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved32_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PORTA_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PORTB_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PORTC_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PORTD_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PORTE_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(LLWU0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(I2S0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(USB0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(ADC0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(LPTMR0_IRQn)] = lptmr0_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(RTC_Seconds_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(INTMUX0_0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(INTMUX0_1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(INTMUX0_2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(INTMUX0_3_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(LPTMR1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved49_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved50_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved51_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(LPSPI2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(LPUART2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(EMVSIM0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(LPI2C2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(TSI0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PMC_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(FTFA_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SCG_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(WDOG0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DAC0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(TRNG_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(RCM_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(CMP0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(CMP1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(RTC_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved67_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved68_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved69_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved70_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved71_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved72_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved73_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved74_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved75_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved76_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved77_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved78_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved79_IRQn)] = unexpected_irq_handler
};

/**
 * SoC configuration in Flash
 */
static const NV_Type nv_cfmconfig __attribute__ ((section(".FlashConfig"))) = {
    .BACKKEY3 = 0xff,
    .BACKKEY2 = 0xff,
    .BACKKEY1 = 0xff,
    .BACKKEY0 = 0xff,
    .BACKKEY7 = 0xff,
    .BACKKEY6 = 0xff,
    .BACKKEY5 = 0xff,
    .BACKKEY4 = 0xff,
    .FPROT3 = 0xff,
    .FPROT2 = 0xff,
    .FPROT1 = 0xff,
    .FPROT0 = 0xff,
    .FSEC = 0xfe,
    .FOPT = 0x3d,
	.padding = 0xffff
};
