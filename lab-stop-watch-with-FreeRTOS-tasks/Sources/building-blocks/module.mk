#
# module-level build makefile
#
# Author: German Rivera
#

local_src := $(subdirectory)/atomic_utils.c \
             $(subdirectory)/byte_ring_buffer.c \
             $(subdirectory)/cortex_m_startup.c \
             $(subdirectory)/cpu_reset_counter.c \
             $(subdirectory)/event_set.c \
             $(subdirectory)/hw_timer_driver.c \
	     $(subdirectory)/$(MCU_CHIP)_interrupt_vector_table.c \
             $(subdirectory)/memory_protection_unit.c \
             $(subdirectory)/mem_utils.c \
             $(subdirectory)/power_utils.c \
             $(subdirectory)/printf_utils.c \
             $(subdirectory)/rtos_wrapper_FreeRTOS.c \
             $(subdirectory)/runtime_checks.c \
             $(subdirectory)/serial_console.c \
             $(subdirectory)/stack_trace.c \
             $(subdirectory)/time_utils.c \
             $(subdirectory)/watchdog.c

$(eval $(call make-library, $(subdirectory)/building-blocks.a, $(local_src)))

