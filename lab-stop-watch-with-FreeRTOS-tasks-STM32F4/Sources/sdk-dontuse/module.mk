#
# module-level build makefile
#
# Author: German Rivera
#

local_src := $(subdirectory)/board/board.c \
             $(subdirectory)/board/clock_config.c \
             $(subdirectory)/board/pin_mux.c \
             $(subdirectory)/drivers/fsl_clock.c \
             $(subdirectory)/drivers/fsl_common.c \
             $(subdirectory)/drivers/fsl_flash.c \
             $(subdirectory)/drivers/fsl_gpio.c \
             $(subdirectory)/drivers/fsl_lpuart.c \
             $(subdirectory)/drivers/fsl_smc.c \
	     $(subdirectory)/MCU/system_MKL28Z7.c \
	     $(subdirectory)/utilities/fsl_debug_console.c

local_subdirs := $(subdirectory)/board \
	         $(subdirectory)/drivers \
	         $(subdirectory)/utilities \
	         $(subdirectory)/MCU

create-local-output-subdirs :=				\
	$(shell for f in $(local_subdirs);		\
		do					\
		  $(TEST) -d $$f || $(MKDIR) $$f;	\
		done)

$(eval $(call make-library, $(subdirectory)/sdk.a, $(local_src)))

