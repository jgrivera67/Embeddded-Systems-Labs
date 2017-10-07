/**
 * @file arm_cmsis.h
 *
 * ARM CMSIS (Cortex Microcontroller Software Interface Standard)
 *
 * @author: German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_ARM_CMSIS_H_
#define SOURCES_BUILDING_BLOCKS_ARM_CMSIS_H_

#if defined(KL25Z_MCU)
#include <MKL25Z4.h>
#include <core_cm0plus.h>
#define asm __ASM
#elif defined(KL28Z_MCU)
#include <MKL28Z7.h>
#include <core_cm0plus.h>
#define asm __ASM
#elif defined(K64F_MCU)
#include <MK64F12.h>
#include <core_cm4.h>
#else
#error "No Microcontroller defined"
#endif

#include <core_cmInstr.h>
#include <core_cmFunc.h>

#endif /* SOURCES_BUILDING_BLOCKS_ARM_CMSIS_H_ */
