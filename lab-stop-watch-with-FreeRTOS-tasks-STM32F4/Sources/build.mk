#
# Application top-level build makefile
#
# Author: German Rivera
#
# NOTE: This Makefile must be run only from the obj directory. It is to be
# invoked from the top-level makefile.
#
ifndef APPLICATION
    $(error APPLICATION not defined)
endif

ifndef PLATFORM
    $(error PLATFORM not defined)
endif

ifndef BUILD_FLAVOR
    $(error BUILD_FLAVOR not defined)
endif

BASE_DIR := $(dir $(CURDIR))
SOURCE_DIR := $(BASE_DIR)Sources
GIT_COMMIT := $(shell git describe --always --dirty)

#
# Check that this makefile is not being run from the source directory
#
$(if $(filter $(notdir $(SOURCE_DIR)), $(notdir $(CURDIR))), \
     $(error This makefile cannot be run from directory $(CURDIR)))

#
# Tools
#
TOOLCHAIN   ?= arm-none-eabi
CC          = $(TOOLCHAIN)-gcc
CPP         = $(TOOLCHAIN)-cpp
AS          = $(TOOLCHAIN)-gcc -x assembler-with-cpp
LD          = $(TOOLCHAIN)-ld
OBJCOPY     = $(TOOLCHAIN)-objcopy
OBJDUMP     = $(TOOLCHAIN)-objdump
AR          = $(TOOLCHAIN)-ar
RANLIB      = $(TOOLCHAIN)-ranlib

ifeq "$(PLATFORM)" "FRDM-KL28Z"
    MCU_CHIP = KL28Z
    CPU_ARCHITECTURE = arm_cortex_m
    ARM_ARCH = armv6-m
    ARM_CORE = cortex-m0plus
    CODETYPE = thumb
    PLATFORM_CPPFLAGS = -DCPU_MKL28Z512VDC7 \
		        -DFSL_RTOS_FREE_RTOS \
		        -DFRDM_KL28Z \
		        -DFREEDOM \
			-D__CORTEX_SC=0x0

else ifeq "$(PLATFORM)" "STM32F401"
    MCU_CHIP = STM32F4_MCU
    CPU_ARCHITECTURE = arm_cortex_m
    ARM_ARCH = armv7e-m
    ARM_CORE = cortex-m4
    CODETYPE = thumb
    # See https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html
    EXTRA_MCFLAGS = -mfloat-abi=softfp -mfpu=fpv4-sp-d16
else
    $(error unsupported platform $(PLATFORM))
endif

ifndef CPU_ARCHITECTURE
    $(error unsupported platform $(PLATFORM))
endif

ifeq "$(CPU_ARCHITECTURE)" "arm_cortex_m"
    EXTRA_MCFLAGS += -fno-omit-frame-pointer #-mtpcs-leaf-frame #-mtpcs-frame
else
    EXTRA_MCFLAGS += -fno-omit-frame-pointer #-mapcs-frame
endif

LDSCRIPT = $(BASE_DIR)/Sources/MCU/STM32F401VEHx_FLASH.ld


# $(call source-to-object, source-file-list)
source-to-object = $(subst .c,.o,$(filter %.c,$1)) \
		   $(subst .s,.o,$(filter %.s,$1))

# $(subdirectory)
subdirectory = $(patsubst $(SOURCE_DIR)/%/module.mk,%,	\
		 $(word					\
		   $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))

# $(call make-library, library-name, source-file-list)
define make-library
  libraries += $1
  sources   += $2

  $1: $(call source-to-object,$2)
	$(AR) $(ARFLAGS) $$@ $$^
endef

modules      := building-blocks \
                freertos \
		application

#
# NOTE: 'programs' is populated by included
# Applications/*/module.mk makefiles
#
programs     :=

#
# NOTE: 'libraries' is populated by make-library invocations and if
# necessary by included Applications/$(PLATFORM)/*/module.mk makefiles
#
libraries    :=

#
# NOTE: 'sources' is populated by make-library invocations
# and by included Applications/$(PLATFORM)/*/module.mk makefiles
#
sources	     :=

objects      = 	$(call source-to-object,$(sources))
dependencies = 	$(subst .o,.d,$(objects))

hex_files = 	$(subst .elf,.hex,$(programs))
bin_files = 	$(subst .elf,.bin,$(programs))
lst_files = 	$(subst .elf,.lst,$(programs))

include_dirs := $(SOURCE_DIR) \
	        $(SOURCE_DIR)/application \
	        $(SOURCE_DIR)/freertos \
	        $(SOURCE_DIR)/freertos/portable \
	        $(SOURCE_DIR)/freertos/portable \
	        $(SOURCE_DIR)/MCU \
	        $(SOURCE_DIR)/MCU/CMSIS \
	        $(SOURCE_DIR)/sdk \
	        $(SOURCE_DIR)/sdk/board \
	        $(SOURCE_DIR)/sdk/drivers \
	        $(SOURCE_DIR)/sdk/utilities \
	        $(SOURCE_DIR)/sdk/MCU

CPPFLAGS     += $(addprefix -I ,$(include_dirs)) \
		-D$(MCU_CHIP) \
		-DGIT_COMMIT=\"$(GIT_COMMIT)\" \
		$(PLATFORM_CPPFLAGS)

ifeq "$(BUILD_FLAVOR)" "debug"
    CPPFLAGS += -DDEBUG \
		-D_RELIABILITY_CHECKS_ \
		-D_CPU_CYCLES_MEASURE_
    OPT = -O0

else ifeq "$(BUILD_FLAVOR)" "reliability"
    CPPFLAGS += -D_RELIABILITY_CHECKS_ \
		-D_CPU_CYCLES_MEASURE_
    OPT = -O0

else ifeq "$(BUILD_FLAVOR)" "performance"
    CPPFLAGS += -D_CPU_CYCLES_MEASURE_
    #OPT can be -O1, -O2, -Os or -O3
    OPT = -O2
endif

#MCFLAGS = 	-march=$(ARM_ARCH) -mtune=$(ARM_CORE) -m$(CODETYPE) $(EXTRA_MCFLAGS)
MCFLAGS = 	-mcpu=$(ARM_CORE) -m$(CODETYPE) $(EXTRA_MCFLAGS)
ASFLAGS = 	$(MCFLAGS) -g -gdwarf-2 \
		$(CPPFLAGS)
		#-Wa,-amhls=$(<:.s=.lst)

CFLAGS  = 	$(MCFLAGS) $(OPT) -gdwarf-2 \
		-ffunction-sections -fdata-sections \
		-Wmissing-prototypes \
		-Wpointer-arith \
		-Winline \
		-fverbose-asm \
		-Werror \
		-Wstack-usage=224 \
		-Wundef \
		-Wdouble-promotion \
		-ffreestanding \
		$(CPPFLAGS) \
		${COMMON_CFLAGS} ${EXTRA_CFLAGS}
                # -Wa,-ahlms=$(<:.c=.lst)

LDFLAGS = 	$(MCFLAGS) -nostartfiles -T$(LDSCRIPT) \
		-Wl,-Map=$(PLATFORM).map,--cref,--no-warn-mismatch \
		-Xlinker --gc-sections \
		#-nostdlib -v

vpath %.h $(include_dirs)
vpath %.c $(SOURCE_DIR)
vpath %.s $(SOURCE_DIR)

MKDIR := mkdir -p
MV    := mv -f
RM    := rm -f
SED   := sed
TEST  := test

create-output-directories :=				\
	$(shell for f in $(modules);			\
		do					\
		  $(TEST) -d $$f || $(MKDIR) $$f;	\
		done)

all:

include $(patsubst %,$(SOURCE_DIR)/%/module.mk,$(modules))

.PHONY: all
all: $(programs) $(hex_files) $(bin_files) $(lst_files)

.PHONY: libraries
libraries: $(libraries)

.PHONY: clean
clean:
	$(RM) -r *

ifneq "$(MAKECMDGOALS)" "clean"
    -include $(dependencies)
endif

%o : %c
	$(CC) -c $(CFLAGS) $< -o $@
	$(MV) $(<:.c=.lst) $(@:.o=.lst)

%o : %s
	$(AS) -c $(ASFLAGS) $< -o $@
	$(MV) $(<:.s=.lst) $(@:.o=.lst)

GEN_HEADER_DEPENDENCIES = \
	($(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M $< | \
	 $(SED) 's,\($(notdir $*)\.o\) *:,$(dir $@)\1 $@: ,' > $@.tmp); \
	$(MV) $@.tmp $@

%.d: %.c
	@$(GEN_HEADER_DEPENDENCIES)

%.d: %.s
	@$(GEN_HEADER_DEPENDENCIES)

#%elf: $(OBJS)
#	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

%lst: %elf
	$(OBJDUMP) -dSst $< > $@

%bin: %elf
	$(OBJCOPY) -O binary -S $< $@

%srec: %elf
	$(OBJCOPY) -O srec -S $< $@

%hex: %elf
	$(OBJCOPY) -O ihex -S $< $@

.PHONY: list_predefined_macros
list_predefined_macros:
	touch ~/tmp/foo.h; ${CPP} -dM ~/tmp/foo.h; rm ~/tmp/foo.h

