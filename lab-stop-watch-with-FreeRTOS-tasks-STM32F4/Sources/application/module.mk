#
# Application main module build makefile
#
# Author: German Rivera
#
local_pgm  := $(subdirectory)/$(APPLICATION).elf
local_src  := $(subdirectory)/main.c

local_objs := $(call source-to-object,$(local_src))

programs   += $(local_pgm)
sources    += $(local_src)
libraries +=

$(local_pgm): $(local_objs) $(libraries)
	$(CC) $(LDFLAGS) $+ -o $@

