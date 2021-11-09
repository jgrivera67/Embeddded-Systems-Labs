/**
 * @file arm_cortex_m_defs.h
 *
 * ARM Cortex-M architecture specific macros
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_ARM_CORTEX_M_DEFS_H_
#define SOURCES_BUILDING_BLOCKS_ARM_CORTEX_M_DEFS_H_

#include <stdint.h>
#include "arm_cmsis.h"

/**
 * Size of of the "bl" instruction in bytes for ARM thumb-2
 */
#define BL_INSTRUCTION_SIZE 4

/*
 * Bit masks for the Cortex-M PRIMASK register
 */
#define CPU_REG_PRIMASK_PM_MASK   0x1

/*
* Bit masks for the IPSR register
*/
#define CPU_REG_IPSR_EXCEPTION_NUMBER_MASK   0x3F

/**
 * Values that LR can be set to, to return from an exception:
 */
#define CPU_EXC_RETURN_TO_HANDLER_MODE                UINT32_C(0xFFFFFFF1)
#define CPU_EXC_RETURN_TO_THREAD_MODE_USING_MSP       UINT32_C(0xFFFFFFF9)
#define CPU_EXC_RETURN_TO_THREAD_MODE_USING_PSP       UINT32_C(0xFFFFFFFD)
#define CPU_EXC_RETURN_TO_THREAD_MODE_USING_PSP_FPU   UINT32_C(0xFFFFFFED)

/**
 * Tell if a return address is one of the exception return special values
 */
#define IS_CPU_EXCEPTION_RETURN(_return_address) \
        ((_return_address) >= CPU_EXC_RETURN_TO_THREAD_MODE_USING_PSP_FPU)

/**
 * Remove the thumb-mode flag from an ARM Cortex-M code address
 */
#define GET_CODE_ADDRESS(_addr)   ((uintptr_t)(_addr) & ~0x1)

/**
 * Calculate the call address given a return address for ARM Cortex-M
 */
#define GET_CALL_ADDRESS(_return_address) \
    (GET_CODE_ADDRESS(_return_address) - BL_INSTRUCTION_SIZE)

/**
 * Tell if a CPU interrupt mask indicates that interrupts are disabled at CPU
 */
#define __CPU_INTERRUPTS_ARE_DISABLED(_reg_primask_value) \
        (((_reg_primask_value) & CPU_REG_PRIMASK_PM_MASK) != 0)

/**
 * Check if interrupts are disabled on the CPU
 */
#define CPU_INTERRUPTS_ARE_DISABLED() \
        __CPU_INTERRUPTS_ARE_DISABLED(__get_PRIMASK())

/**
 * Tell if a CPU interrupt mask indicates that interrupts are enabled at CPU
 */
#define __CPU_INTERRUPTS_ARE_ENABLED(_reg_primask_value) \
        (!__CPU_INTERRUPTS_ARE_DISABLED(_reg_primask_value))

/**
 * Check if interrupts are enabled on the CPU
 */
#define CPU_INTERRUPTS_ARE_ENABLED() \
        (!CPU_INTERRUPTS_ARE_DISABLED())

/**
 * Tell if an IPSR register value indicates that the CPU is in thread mode
 */
#define __CPU_MODE_IS_THREAD(_reg_ipsr_value) \
        (((_reg_ipsr_value) & CPU_REG_IPSR_EXCEPTION_NUMBER_MASK) == 0)

/**
 * Tell if an IPSR register value indicates that the CPU is in handler mode
 */
#define __CPU_MODE_IS_HANDLER(_reg_ipsr_value) \
        (!__CPU_MODE_IS_THREAD(_reg_ipsr_value))

/**
 * Check if the caller is a thread (CPU is in thread mode)
 */
#define CPU_MODE_IS_THREAD() \
        __CPU_MODE_IS_THREAD(__get_IPSR())

/**
 * Check if the caller is an interrupt or exception handler (CPU is in handler
 * mode)
 */
#define CPU_MODE_IS_HANDLER() \
        __CPU_MODE_IS_HANDLER(__get_IPSR())

/**
 * Tell if a CONTROL register value indicates that the CPU is using the
 * MSP stack pointer
 */
#define __CPU_USING_MSP_STACK_POINTER(_reg_control_value) \
        (((_reg_control_value) & CPU_REG_CONTROL_SPSEL_MASK) == 0)

/**
 * Tell if a CONTROL register value indicates that the CPU is using the
 * PSP stack pointer
 */
#define __CPU_USING_PSP_STACK_POINTER(_reg_control_value) \
        (!__CPU_USING_MSP_STACK_POINTER(_reg_control_value))

/**
 * Check if the caller is using the MSP stack pointer
 */
#define CPU_USING_MSP_STACK_POINTER() \
        __CPU_USING_MSP_STACK_POINTER(__get_CONTROL())

/**
 * Check if the caller is using the PSP stack pointer
 */
#define CPU_USING_PSP_STACK_POINTER() \
        __CPU_USING_PSP_STACK_POINTER(__get_CONTROL())

/**
 * Check if the caller is an RTOS task
 */
#define CALLER_IS_THREAD() \
		CPU_USING_PSP_STACK_POINTER()

/**
 * Check if the caller is an interrupt or exception handler
 * (other than the reset handler)
 */
#define CALLER_IS_ISR() \
		(CPU_USING_MSP_STACK_POINTER() && CPU_MODE_IS_HANDLER())

/**
 * Check if the caller is the reset handler
 */
#define CALLER_IS_RESET_HANDLER() \
		(CPU_USING_MSP_STACK_POINTER() && CPU_MODE_IS_THREAD())

/**
 * Capture current value of the ARM core frame pointer (r7) register
 */
#define CAPTURE_ARM_FRAME_POINTER_REGISTER(_fp_value) \
        asm volatile ("mov %[fp_value], r7" : [fp_value] "=r" (_fp_value))

/**
 * Capture current value of the ARM core LR (r14) register
 */
#define CAPTURE_ARM_LR_REGISTER(_lr_value) \
    asm volatile ("mov %[lr_value], lr" : [lr_value] "=r" (_lr_value))

/**
 * Capture current value of the ARM core SP (r13) register
 */
#define CAPTURE_ARM_SP_REGISTER(_sp_value) \
    asm volatile ("mov %[sp_value], sp" : [sp_value] "=r" (_sp_value))

/**
 * "bl" instruction (32-bit instruction) opcode first-half mask
 */
#define  IS_BL32_FIRST_HALF(_instr) \
        (((_instr) & 0xF000) == 0xF000 && \
         ((_instr) & 0x800) == 0x0)

/**
 * "bl" instruction (32-bit instruction) opcode second-half mask
 */
#define  IS_BL32_SECOND_HALF(_instr) \
        (((_instr) & 0xD000) == 0xD000)

/**
 * "blx" instruction opcode mask (16-bit instruction)
 */
#define  IS_BLX(_instr) \
        (((_instr) & 0xFF80) == 0x4780)

/*
 * Bitmasks to decode the op-code and operand fields of a THUMB instruction
 */
#define THUMB_INSTR_OP_CODE_MASK    0xFF00
#define THUMB_INSTR_OPERANDS_MASK    0x00FF

/**
 * "add r7, sp, #imm8" instruction immediate operand mask
 * (since this operand must be a multiple of 4, it
 * is encoded in the instruction shifted 2 bits to the
 * right)
 */
#define ADD_SP_IMMEDITATE_OPERAND_MASK    0x00FF

/**
 * 'sub sp, #imm7' instruction immediate operand mask
 * (since this operand must be a multiple of 4, it
 * is encoded in the instruction shifted 2 bits to the
 * right)
 */
#define SUB_SP_IMMEDITATE_OPERAND_MASK    0x007F

/**
 * 'push' instruction modifier "append lr to reg list" mask
 */
#define  PUSH_OPERAND_INCLUDES_LR_MASK    0x0100

/**
 * 'push' instruction operand register list mask
 */
#define  PUSH_OPERAND_REG_LIST_MASK        0x00FF

/**
 * Tell if it is the 'add r7, sp, #imm8' instruction
 */
#define IS_ADD_R7_SP_IMMEDITATE(_instr) \
        (((_instr) & THUMB_INSTR_OP_CODE_MASK) == 0xAF00)

/**
 * Tell if it is the 'sub sp, #imm7' instruction
 */
#define IS_SUB_SP_IMMEDITATE(_instr) \
        (((_instr) & THUMB_INSTR_OP_CODE_MASK) == 0xB000 && \
         ((_instr) & 0x80))

/**
 * Tell if it is the 'push {...,r7,...}' instruction
 */
#define IS_PUSH_R7(_instr) \
        (((_instr) & 0xFE00) == 0xB400 && \
         ((_instr) & 0x80))

/*
 * Bit masks for the IPSR register
 */
#define CPU_REG_IPSR_EXCEPTION_NUMBER_MASK   0x3F

/*
 * Bit masks for the EPSR register
 */
#define CPU_REG_EPSR_THUMB_STATE_MASK    (0x1 << 24)

/*
 * Bit masks for the PRIMASK register
 */
#define CPU_REG_PRIMASK_PM_MASK   0x1

/*
 * Bit masks for the CONTROL register
 */
#define CPU_REG_CONTROL_nPRIV_MASK   (0x1 << 0)
#define CPU_REG_CONTROL_SPSEL_MASK   (0x1 << 1)
#define CPU_REG_CONTROL_FPCA_MASK	 (0x1 << 2)

/**
* Cortex-M SCB ICSR register address
*/
#define CPU_SCB_ICSR_REGISTER_ADDR   0xE000ED04

/**
 * Mask for the PendSV set-pending bit in the SCB ICSR register
 */
#define CPU_SCB_ICSR_PENDSVSET_MASK  (0x1 << 28)

/**
 * ARM thumb instruction
 */
typedef uint16_t thumb_instruction_t;

#endif /* SOURCES_BUILDING_BLOCKS_ARM_CORTEX_M_DEFS_H_ */
