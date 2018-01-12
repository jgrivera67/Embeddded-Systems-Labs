#
# application top-level makefile
#
# Author: German Rivera
#
# NOTE: additional CFLAGS can be specified in the command line like this:
# make EXTRA_CFLAGS=...
#

#
# Application binary name
#
APPLICATION := stop_watch_with_freertos_tasks

#
# PLATFORM values: FRDM-KL28Z, STM32F401
# This variable must be set via an environment variable or command-line make variable
#
PLATFORM ?= STM32F401

#
# BUILD_FLAVOR values: debug, reliability, performance
# This variable must be set via an environment variable or command-line make variable
#
BUILD_FLAVOR ?= debug

SOURCE_DIR := Sources
OBJECT_DIR := $(PLATFORM)-obj-$(BUILD_FLAVOR)
UNIT_TESTS_DIR := unit_tests
SCRIPTS_DIR := scripts
DOC_DIR := doc

RUN_BUILD_MAKEFILE := $(MAKE) --directory=$(OBJECT_DIR) \
			      --file=../$(SOURCE_DIR)/build.mk \
			      $(BUILD_TARGET) \
			      APPLICATION='$(APPLICATION)' \
			      PLATFORM='$(PLATFORM)' \
			      BUILD_FLAVOR='$(BUILD_FLAVOR)' \
			      EXTRA_CFLAGS='$(EXTRA_CFLAGS)' \

RUN_TESTS_MAKEFILE := $(MAKE) --directory=$(UNIT_TESTS_DIR)

RUN_DOC_MAKEFILE := $(MAKE) --directory=$(DOC_DIR)

COMMON_CFLAGS = -Wall -Wstrict-prototypes -fms-extensions -Wextra -Wformat \
		-Wunreachable-code -Wshadow \
	        -std=gnu11 -g3

.PHONY: build create_object_dir run_tests doc \
	clean

build: create_object_dir
	@$(RUN_BUILD_MAKEFILE) COMMON_CFLAGS='$(COMMON_CFLAGS)'

rebuild: create_object_dir
	@$(RUN_BUILD_MAKEFILE) clean
	@$(RUN_BUILD_MAKEFILE) COMMON_CFLAGS='$(COMMON_CFLAGS)'

create_object_dir:
	@$(shell test -d $(OBJECT_DIR) || mkdir $(OBJECT_DIR))

update_flash: build
	cp $(BIN_FILE) /media/$(USER)/MBED
	sync

run_tests:
	$(RUN_TESTS_MAKEFILE) COMMON_CFLAGS='$(COMMON_CFLAGS)'

doc:
	$(RUN_DOC_MAKEFILE)

clean:
	-@$(RUN_BUILD_MAKEFILE) clean
	-@$(RUN_TESTS_MAKEFILE) clean
	-@$(RUN_DOC_MAKEFILE) clean_obj

