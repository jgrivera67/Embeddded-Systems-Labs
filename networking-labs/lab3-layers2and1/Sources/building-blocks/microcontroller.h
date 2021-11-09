/**
 * @file microcontroller.h
 *
 * Microcontroller specific declarations
 *
 * @author: German Rivera
 */
#ifndef MICROCONTROLLER_H_
#define MICROCONTROLLER_H_

#include <stdint.h>
#define K64F_MCU
#include "arm_cmsis.h"
#include "arm_cortex_m_defs.h"

/**
 * NOR Flash base address
 */
#define MCU_FLASH_BASE_ADDR    UINT32_C(0x0)

#if defined(KL25Z_MCU)
/**
 * NOR Flash size in bytes
 */
#define MCU_FLASH_SIZE    (UINT32_C(128) * 1024)

/**
 * SRAM base address
 */
#define MCU_SRAM_BASE_ADDR    UINT32_C(0x1FFFF000)

/**
 * SRAM size in bytes
 */
#define MCU_SRAM_SIZE    (UINT32_C(16) * 1024)

/**
 * Number of interrupt priorities
 */
#define MCU_NUM_INTERRUPT_PRIORITIES 4

/**
 * CPU clock frequency in MHz
 */
#define MCU_CPU_CLOCK_FREQ_IN_MHZ  UINT32_C(48)

#elif defined(K64F_MCU)

/**
 * NOR Flash size in bytes
 */
#define MCU_FLASH_SIZE    (UINT32_C(1024) * 1024)

/**
 * SRAM base address
 */
#define MCU_SRAM_BASE_ADDR    UINT32_C(0x1FFF0000)

/**
 * SRAM size in bytes
 */
#define MCU_SRAM_SIZE    (UINT32_C(256) * 1024)

/**
 * Number of interrupt priorities
 */
#define MCU_NUM_INTERRUPT_PRIORITIES 16

/**
 * CPU clock frequency in MHz
 */
#define MCU_CPU_CLOCK_FREQ_IN_MHZ  UINT32_C(120)

#else
#error "No Microcontroller defined"
#endif

/*
 * MMIO Ranges
 */
#define MCU_PERIPHERAL_BRIDGE_MIN_ADDR      UINT32_C(0x40000000)

#define MCU_PERIPHERAL_BRIDGE_MAX_ADDR      UINT32_C(0x400FFFFF)
#define MCU_PRIVATE_PERIPHERALS_MIN_ADDR    UINT32_C(0xE0000000)
#define MCU_PRIVATE_PERIPHERALS_MAX_ADDR    UINT32_C(0xE00FFFFF)
#define MCU_MTB_MIN_ADDR                    UINT32_C(0xF0000000)
#define MCU_MTB_MAX_ADDR                    UINT32_C(0xF0000FFF)

/**
 * CPU clock frequency in Hz
 */
#define MCU_CPU_CLOCK_FREQ_IN_HZ \
        (MCU_CPU_CLOCK_FREQ_IN_MHZ * UINT32_C(1000000))

/**
 * System clock frequency
 */
#define MCU_SYSTEM_CLOCK_FREQ_IN_HZ		MCU_CPU_CLOCK_FREQ_IN_HZ

/**
 * Bus clock frequency
 */
#define MCU_BUS_CLOCK_FREQ_IN_HZ 		(MCU_CPU_CLOCK_FREQ_IN_HZ / 2)

#define MCU_HIGHEST_INTERRUPT_PRIORITY  0
#define MCU_LOWEST_INTERRUPT_PRIORITY   (MCU_NUM_INTERRUPT_PRIORITIES - 1)


/**
 * Memory protection unit (MPU) region alignment in bytes
 */
#define MPU_REGION_ALIGNMENT	UINT32_C(32)

/*
 * CPU reset causes
 */
enum cpu_reset_causes {
    INVALID_RESET_CAUSE =            0x0,
    POWER_ON_RESET =                 0x1,
    EXTERNAL_PIN_RESET =             0x2,
    WATCHDOG_RESET =                 0x3,
    SOFTWARE_RESET =                 0x4,
    LOCKUP_EVENT_RESET =             0x5,
    EXTERNAL_DEBUGGER_RESET =        0x6,
    OTHER_HW_REASON_RESET =          0x7,
    STOP_ACK_ERROR_RESET  =          0x8
};

void reset_cpu(void);

enum cpu_reset_causes find_cpu_reset_cause(void);

#endif /* MICROCONTROLLER_H_ */
