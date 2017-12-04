#
# module-level build makefile
#
# Author: German Rivera
#

local_src := $(subdirectory)/croutine.c \
             $(subdirectory)/event_groups.c \
             $(subdirectory)/heap_4.c \
             $(subdirectory)/list.c \
             $(subdirectory)/queue.c \
             $(subdirectory)/tasks.c \
             $(subdirectory)/timers.c \
             $(subdirectory)/portable/fsl_tickless_systick.c \
             $(subdirectory)/portable/port.c

local_subdirs := $(subdirectory)/portable

create-local-output-subdirs :=				\
	$(shell for f in $(local_subdirs);		\
		do					\
		  $(TEST) -d $$f || $(MKDIR) $$f;	\
		done)

$(eval $(call make-library, $(subdirectory)/freertos.a, $(local_src)))

