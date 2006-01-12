##################################################################
#
# Makefile for MacOS X to build MacOS-specific API test program,
# Also targets to setup vanilla source for MPW are included.
# This should be executed in this directory, as 
# "cd ft2demos ; make -C mac".
#
# most part is taken from ft2demos/Makefile, for UNIX platform.
#

all: exes

empty :=
space := $(empty) $(empty)

ifndef TOP_DIR
  TOP_DIR := ../../freetype2
endif

ifndef TOP_DIR_2_MAC
  ifdef TOP_DIR_2
    TOP_DIR_2_MAC := $(TOP_DIR_2)/mac
  else
    TOP_DIR_2_MAC := .
  endif
endif

ifndef TOP_DIR_2
  TOP_DIR_2 := ..
endif

ifndef CONFIG_MK
  PROJECT   := freetype
  CONFIG_MK := $(TOP_DIR)/config.mk
endif

ifeq ($(wildcard $(CONFIG_MK)),)
  no_config_mk := 1
endif

ifdef no_config_mk
  exes:
	@echo Please compile the library before the demo programs!
  clean distclean:
	@echo "I need \`$(subst /,$(SEP),$(TOP_DIR)/config.mk)' to do that!"
else
  include $(CONFIG_MK)
  have_makefile := $(strip $(wildcard Makefile))
  TOP_DIR   := $(shell cd $(TOP_DIR); pwd)
  TOP_DIR_2 := $(shell cd $(TOP_DIR_2); pwd)
  ifneq ($(have_makefile),)
    BIN_DIR := $(TOP_DIR_2)/bin
    OBJ_DIR := $(TOP_DIR_2)/obj
  else
    BIN_DIR := ..
    OBJ_DIR := ..
  endif

  GRAPH_DIR := $(TOP_DIR_2)/graph
  include $(GRAPH_DIR)/rules.mk
 
  ifeq ($(TOP_DIR),..)
    SRC_DIR := src
  else
    SRC_DIR := $(TOP_DIR_2)/src
  endif

  FT_INCLUDES := $(OBJ_BUILD) $(BUILD_DIR) $(TOP_DIR)/include $(SRC_DIR)
  INCLUDES := $(subst /,$(COMPILER_SEP),$(FT_INCLUDES))
  COMPILE = $(CC) $(CFLAGS) $(INCLUDES:%=$I%)
  FTLIB := $(LIB_DIR)/$(LIBRARY).$A
  ifeq ($(PLATFORM),unix)
    MATH := -lm
  endif
  ifeq ($(PLATFORM),unixdev)
    MATH := -lm
  endif
  LINK_ITEMS = $T$(subst /,$(COMPILER_SEP),$@ $<)

  CC        = $(CCraw)
  LINK_CMD  = $(subst /,$(SEP),$(OBJ_BUILD)/libtool) \
		--mode=link $(CC) \
		$(subst /,$(COMPILER_SEP),$(LDFLAGS))
  LINK_LIBS = $(subst /,$(COMPILER_SEP),$(FTLIB) $(EFENCE)) $(MATH)
  LINK_COMMON = $(LINK_CMD) \
		$(LINK_ITEMS) $(subst /,$(COMPILER_SEP),$(COMMON_OBJ)) \
		$(LINK_LIBS)

  .PHONY: exes clean distclean

  E :=
  EXES := ftoldmac
  exes: $(EXES:%=$(BIN_DIR)/%$E)

  $(BIN_DIR)/ftoldmac$E: $(OBJ_DIR)/ftoldmac.$(SO) $(FTLIB) $(COMMON_OBJ)
	$(LINK_COMMON)

  $(OBJ_DIR)/ftoldmac.$(SO): $(TOP_DIR_2_MAC)/ftoldmac.c
	$(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)

  COMMON_OBJ := $(OBJ_DIR)/common.$(SO)
  $(COMMON_OBJ): $(SRC_DIR)/common.c
	$(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)

  ################################################################
  # Targets to setup MPW 
  #

  clean:
	rm -f \
		$(TOP_DIR_2)/ftoldmac.*.make \
		$(OBJ_DIR)/*.NJ \
		$(OBJ_DIR)/*.makeout \
		$(OBJ_DIR)/*.o $(OBJ_DIR)/*.exe

  makefiles:
	python $(TOP_DIR_2_MAC)/ascii2mpw.py \
		> $(TOP_DIR_2)/ftoldmac.m68k_far.make \
		< $(TOP_DIR_2_MAC)/ftoldmac.m68k_far.make.txt
	python $(TOP_DIR_2_MAC)/ascii2mpw.py \
		> $(TOP_DIR_2)/ftoldmac.ppc_classic.make \
		< $(TOP_DIR_2_MAC)/ftoldmac.ppc_classic.make.txt
	find $(TOP_DIR_2) -name '*.make' | \
	xargs /Developer/Tools/SetFile -a l -c "MPS " -t TEXT

  resource:
	find $(SRC_DIR) $(TOP_DIR_2_MAC) -name '*.[ch]' | \
	xargs /Developer/Tools/SetFile -a l -c "MPS " -t TEXT
endif
