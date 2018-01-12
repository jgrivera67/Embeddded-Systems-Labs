/**
 * @file system_clocks.c
 *
 * System clocks initialization implementation
 *
 * @author German Rivera
 */
#include "system_clocks.h"
#include <stdint.h>
#include <MCU/stm32f401xe.h>
#include "io_utils.h"


/**
 *         The system Clock is configured as follow :
 *            System Clock source            = PLL (HSE)
 *            SYSCLK(Hz)                     = 100000000
 *            HCLK(Hz)                       = 100000000
 *            AHB Prescaler                  = 1
 *            APB1 Prescaler                 = 2
 *            APB2 Prescaler                 = 1
 *            HSE Frequency(Hz)              = 8000000
 *            PLL_M                          = 8
 *            PLL_N                          = 400
 *            PLL_P                          = 4
 *            VDD(V)                         = 3.3
 *            Main regulator output voltage  = Scale1 mode
 *            Flash Latency(WS)              = 3
 */
static void pll_init(void)
{
    uint32_t reg_value;

    /* Enable HSE external oscillator */
    RCC->CR |= RCC_CR_HSEBYP;

    /* Enable HSE crystal oscillator */
    RCC->CR |= RCC_CR_HSEON;
    do {
        reg_value = RCC->CR;
    } while ((reg_value & RCC_CR_HSERDY) == 0);

    /* Set FLASH latency */
    reg_value = FLASH->ACR;
    SET_BIT_FIELD(reg_value, FLASH_ACR_LATENCY_Msk, FLASH_ACR_LATENCY_Pos,
                  FLASH_ACR_LATENCY_3WS);
    FLASH->ACR = reg_value;

    /* Main PLL configuration and activation */
    reg_value = RCC->PLLCFGR;
    reg_value |= RCC_PLLCFGR_PLLSRC_Msk;
    /* PLL, PLLI2S and PLLSAI division factor by 8 */
    SET_BIT_FIELD(reg_value, RCC_PLLCFGR_PLLM_Msk, RCC_PLLCFGR_PLLM_Pos, 8);
    SET_BIT_FIELD(reg_value, RCC_PLLCFGR_PLLN_Msk, RCC_PLLCFGR_PLLN_Pos, 400);
    RCC->PLLCFGR = reg_value;

    /* Main PLL division factor for PLLP output by 4 */
    reg_value = RCC->PLLCFGR;
    SET_BIT_FIELD(reg_value, RCC_PLLCFGR_PLLP_Msk, RCC_PLLCFGR_PLLP_Pos, 1);
    RCC->PLLCFGR = reg_value;

    /* Enable PLL: */
    RCC->CR |= RCC_CR_PLLON;
    do {
        reg_value = RCC->CR;
    } while ((reg_value & RCC_CR_PLLRDY) == 0);

    /* Sysclk activation on the main PLL */
    reg_value = RCC->CFGR;
    SET_BIT_FIELD(reg_value, RCC_CFGR_HPRE_Msk, RCC_CFGR_HPRE_Pos, RCC_CFGR_HPRE_DIV1);
    RCC->CFGR = reg_value;

    /* Select PLL as system clock */
    reg_value = RCC->CFGR;
    SET_BIT_FIELD(reg_value, RCC_CFGR_SW_Msk, RCC_CFGR_SW_Pos, RCC_CFGR_SW_PLL);
    RCC->CFGR = reg_value;
    do {
        reg_value = RCC->CFGR;
    } while (GET_BIT_FIELD(reg_value, RCC_CFGR_SWS_Msk, RCC_CFGR_SWS_Pos) !=
	     RCC_CFGR_SWS_PLL);

    /* Set APB1 & APB2 prescalers */
    reg_value = RCC->CFGR;
    SET_BIT_FIELD(reg_value, RCC_CFGR_PPRE1_Msk, RCC_CFGR_PPRE1_Pos,
	          RCC_CFGR_PPRE1_DIV2);
    SET_BIT_FIELD(reg_value, RCC_CFGR_PPRE2_Msk, RCC_CFGR_PPRE2_Pos,
	          RCC_CFGR_PPRE2_DIV1);
    RCC->CFGR = reg_value;
}


void system_clocks_init(void)
{
    /* FPU settings ------------------------------------------------------------*/
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
#endif

    /* Reset the RCC clock configuration to the default reset state ------------*/
    /* Set HSION bit */
    RCC->CR |= (uint32_t)0x00000001;

    /* Reset CFGR register */
    RCC->CFGR = 0x00000000;

    /* Reset HSEON, CSSON and PLLON bits */
    RCC->CR &= (uint32_t)0xFEF6FFFF;

    /* Reset PLLCFGR register */
    RCC->PLLCFGR = 0x24003010;

    /* Reset HSEBYP bit */
    RCC->CR &= (uint32_t)0xFFFBFFFF;

    /* Disable all interrupts */
    RCC->CIR = 0x00000000;

    /*
     * Initialize PLL:
     */
    pll_init();
}


