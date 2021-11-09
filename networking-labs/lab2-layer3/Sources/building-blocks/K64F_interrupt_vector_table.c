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


/**
 * Default Hard fault exception handler
 */
void HardFault_Handler(void)
{
    D_ASSERT(false);

    /*
     * Trap the processor in a dummy infinite loop
     */
    for ( ; ; )
        ;
}


/**
 * Default MemoryManagement fault exception handler
 */
void MemoryManagementFault_Handler(void)
{
    D_ASSERT(false);

    /*
     * Trap the processor in a dummy infinite loop
     */
    for ( ; ; )
        ;
}

/**
 * Default Bus fault exception handler
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
 * Default Usage fault exception handler
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
 * Default Debug monitor exception handler
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
    [0] = (void *) (void *)__StackTop,

    /*
     * Processor exceptions
     */
    [1] = Reset_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(NonMaskableInt_IRQn)] = NMI_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(HardFault_IRQn)] = HardFault_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(MemoryManagement_IRQn)] = MemoryManagementFault_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(BusFault_IRQn)] = BusFault_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(UsageFault_IRQn)] = UsageFault_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SVCall_IRQn)] = SVC_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DebugMonitor_IRQn)] = DebugMonitor_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PendSV_IRQn)] = PendSV_Handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SysTick_IRQn)] = SysTick_Handler,

    /*
     * Interrupts external to the Cortex-M core
     */
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA3_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA4_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA5_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA6_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA7_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA8_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA9_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA10_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA11_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA12_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA13_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA14_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA15_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DMA_Error_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(MCM_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(FTFE_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Read_Collision_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(LVD_LVW_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(LLWU_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(WDOG_EWM_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(RNG_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(I2C0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(I2C1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SPI0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SPI1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(I2S0_Tx_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(I2S0_Rx_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(UART0_LON_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(UART0_RX_TX_IRQn)] = uart0_rx_tx_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(UART0_ERR_IRQn)] = uart0_error_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(UART1_RX_TX_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(UART1_ERR_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(UART2_RX_TX_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(UART2_ERR_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(UART3_RX_TX_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(UART3_ERR_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(ADC0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(CMP0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(CMP1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(FTM0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(FTM1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(FTM2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(CMT_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(RTC_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(RTC_Seconds_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PIT0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PIT1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PIT2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PIT3_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PDB0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(USB0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(USBDCD_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(Reserved71_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DAC0_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(MCG_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(LPTMR0_IRQn)] = lptmr0_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PORTA_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PORTB_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PORTC_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PORTD_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(PORTE_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SWI_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SPI2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(UART4_RX_TX_IRQn)] = uart4_rx_tx_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(UART4_ERR_IRQn)] = uart4_error_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(UART5_RX_TX_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(UART5_ERR_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(CMP2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(FTM3_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(DAC1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(ADC1_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(I2C2_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(CAN0_ORed_Message_buffer_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(CAN0_Bus_Off_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(CAN0_Error_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(CAN0_Tx_Warning_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(CAN0_Rx_Warning_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(CAN0_Wake_Up_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(SDHC_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(ENET_1588_Timer_IRQn)] = unexpected_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(ENET_Transmit_IRQn)] = ethernet_mac0_tx_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(ENET_Receive_IRQn)] = ethernet_mac0_rx_irq_handler,
    [IRQ_NUMBER_TO_VECTOR_NUMBER(ENET_Error_IRQn)] = ethernet_mac0_error_irq_handler,
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
    .FOPT = 0xff,
    .FEPROT = 0xff,
    .FDPROT = 0xff
};
