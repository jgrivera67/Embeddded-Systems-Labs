/**
 * @file hw_timer.c
 *
 * Hardware timer driver implementation
 *
 * @author German Rivera
 */
#include "hw_timer_driver.h"
#include "io_utils.h"
#include "arm_cmsis.h"
#include "time_utils.h"
#include "interrupt_vector_table.h"
#include "rtos_wrapper.h"
#include <stddef.h>

/**
 * Mutable data of the hardware timer device (to be placed in SRAM)
 */
struct hw_timer_device_var {
    /**
     * Flag indicating if hw_timer_init() has been called for this device
     */
    bool initialized;

    /**
     * Pointer to callback function to be invoked every time that the timer
     * fires
     */
    hw_timer_callback_t *callback_func;

    /**
     * Argument to be passed to the callback function
     */
    void *callback_arg;
};

static struct hw_timer_device_var g_hw_timer0_var;

/**
 * Single instance of the hardware timer device (LPTMR0 peripheral in the KL25Z):
 */
const struct hw_timer_device g_hw_timer0 = {
	.signature = HW_TIMER_SIGNATURE,
	.mmio_registers_p = LPTMR0_BASE_PTR,
	.var_p = &g_hw_timer0_var,
};


/**
 * Initializes a hardware timer device
 *
 * @param hw_timer_p 			Pointer to the hardware timer device
 * @param timer_period_ms		Firing period for the timer in milliseconds
 * @param timer_callback_func	Pointer to callback function to invoke when
 * 								the timer fires
 * @param timer_callback_arg	Argument to be passed to the callback fucntion
 */
void hw_timer_init(const struct hw_timer_device *hw_timer_p,
				   uint16_t timer_period_ms,
		           hw_timer_callback_t *timer_callback_func,
				   void *timer_callback_arg)
{
    LPTMR_Type *lptmr_regs_p = hw_timer_p->mmio_registers_p;
    struct hw_timer_device_var *hw_timer_var_p = hw_timer_p->var_p;
	uint32_t reg_value;

	D_ASSERT(hw_timer_p->signature == HW_TIMER_SIGNATURE);
	D_ASSERT(!hw_timer_var_p->initialized);
    D_ASSERT(timer_period_ms != 0);

	hw_timer_var_p->initialized = true;
	hw_timer_var_p->callback_func = timer_callback_func;
	hw_timer_var_p->callback_arg = timer_callback_arg;

	/*
	 * Enable clock for the LPTMR peripheral:
	 */
	reg_value = READ_MMIO_REGISTER(&SIM->SCGC5);
	reg_value |= SIM_SCGC5_LPTMR_MASK;
	WRITE_MMIO_REGISTER(&SIM->SCGC5, reg_value);

	/*
	 * Disable the LPTMR peripheral before configuring it:
	 */
	reg_value = READ_MMIO_REGISTER(&lptmr_regs_p->CSR);
	reg_value &= ~LPTMR_CSR_TEN_MASK;
	WRITE_MMIO_REGISTER(&lptmr_regs_p->CSR, reg_value);

	/*
	 * Clear any pending interrupts and reset CNR:
	 */
	reg_value = READ_MMIO_REGISTER(&lptmr_regs_p->CSR);
	reg_value |= LPTMR_CSR_TCF_MASK;
	WRITE_MMIO_REGISTER(&lptmr_regs_p->CSR, reg_value);

    /*
     * Configure operation mode:
     * - select time counter mode
     * - select timer pin (pulse counter input 0)
     */
	reg_value = READ_MMIO_REGISTER(&lptmr_regs_p->CSR);
	reg_value &= ~LPTMR_CSR_TMS_MASK;
	reg_value |= LPTMR_CSR_TPS(0x0);
	WRITE_MMIO_REGISTER(&lptmr_regs_p->CSR, reg_value);

	/*
	 * Configure Prescale register:
	 * - prescaler bypass
	 * - Use 1KHz low power clock source
	 */
	reg_value = LPTMR_PSR_PRESCALE(0x0) |	/* prescaler value */
	            LPTMR_PSR_PBYP_MASK | 		/* prescaler bypass */
	            LPTMR_PSR_PCS(0x1);    		/* Clock source */
	WRITE_MMIO_REGISTER(&lptmr_regs_p->PSR, reg_value);

	/*
	 * Set timer period in the Compare register:
	 */
	WRITE_MMIO_REGISTER(&lptmr_regs_p->CMR, timer_period_ms);

    /*
     * Enable interrupt generation
     */
	reg_value = READ_MMIO_REGISTER(&lptmr_regs_p->CSR);
	reg_value |= LPTMR_CSR_TIE_MASK;
	WRITE_MMIO_REGISTER(&lptmr_regs_p->CSR, reg_value);

	/*
	 * Enable interrupt in the interrupt controller (NVIC):
	 */

	IRQn_Type irq_number = LPTMR0_IRQn;

    NVIC_SetPriority(irq_number, HW_TIMER_INTERRUPT_PRIORITY);
    NVIC_ClearPendingIRQ(irq_number);
    NVIC_EnableIRQ(irq_number);

    /*
     * Enable the LPTMR peripheral:
     */
	reg_value = READ_MMIO_REGISTER(&lptmr_regs_p->CSR);
	reg_value |= LPTMR_CSR_TEN_MASK;
	WRITE_MMIO_REGISTER(&lptmr_regs_p->CSR, reg_value);
}


static void hw_timer_irq_handler(const struct hw_timer_device *hw_timer_p)
{
	uint32_t reg_value;
    LPTMR_Type *lptmr_regs_p = hw_timer_p->mmio_registers_p;
    struct hw_timer_device_var *hw_timer_var_p = hw_timer_p->var_p;

	D_ASSERT(hw_timer_var_p->initialized);

	/*
     * Clear interrupt source
     */
    reg_value = READ_MMIO_REGISTER(&lptmr_regs_p->CSR);
    reg_value |= LPTMR_CSR_TCF_MASK;
    WRITE_MMIO_REGISTER(&lptmr_regs_p->CSR, reg_value);

	if (hw_timer_var_p->callback_func != NULL) {
		hw_timer_var_p->callback_func(hw_timer_var_p->callback_arg);
	}
}


/**
 * ISR for the hardware timer interrupt
 */
void lptmr0_irq_handler(void)
{
	D_ASSERT(CPU_INTERRUPTS_ARE_ENABLED());

    rtos_enter_isr();
	hw_timer_irq_handler(&g_hw_timer0);
    rtos_exit_isr();
}
