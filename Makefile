all: exes


####################################################################
#
# The `space' variable is used to avoid trailing spaces in defining
# the `T' variable later.
#
empty :=
space := $(empty) $(empty)


####################################################################
#
# TOP_DIR is the directory where the main FreeType source is found,
# as well as the `config.mk' file.
#
# TOP_DIR_2 is the directory is the top of the demonstration
# programs directory.
#
ifndef TOP_DIR
  TOP_DIR := ../freetype2
endif

ifndef TOP_DIR_2
  TOP_DIR_2 := .
endif


######################################################################
#
# MY_CONFIG_MK points to the current `config.mk' to use.  It is
# defined by default as $(TOP_DIR)/config.mk.
#
ifndef CONFIG_MK
  PROJECT   := freetype
  CONFIG_MK := $(TOP_DIR)/config.mk
endif


####################################################################
#
# Check that we have a working `config.mk' in the above directory.
# If not, issue a warning message, then stop there.
#
ifeq ($(wildcard $(CONFIG_MK)),)
  no_config_mk := 1
endif

ifdef no_config_mk

  exes:
	  @echo Please compile the library before the demo programs!
  clean distclean:
	  @echo "I need \`$(subst /,$(SEP),$(TOP_DIR)/config.mk)' to do that!"

else

  ####################################################################
  #
  # Good, now include `config.mk' in order to know how to build
  # object files from sources, as well as other things (compiler
  # flags).
  #
  include $(CONFIG_MK)


  ####################################################################
  #
  # Define a few important variables now.
  #
  ifeq ($(PLATFORM),unix)
    # without absolute paths libtool fails
    TOP_DIR   := $(shell cd $(TOP_DIR); pwd)
    TOP_DIR_2 := $(shell cd $(TOP_DIR_2); pwd)
    BIN_DIR   := $(TOP_DIR_2)/bin
    OBJ_DIR   := $(TOP_DIR_2)/obj
  else
    BIN_DIR := bin
    OBJ_DIR := obj
  endif

  GRAPH_DIR := graph

  ifeq ($(TOP_DIR),..)
    SRC_DIR := src
  else
    SRC_DIR := $(TOP_DIR_2)/src
  endif

  FT_INCLUDES := $(BUILD_DIR) $(TOP_DIR)/include $(SRC_DIR)

  COMPILE = $(CC) $(CFLAGS) $(INCLUDES:%=$I%)

  FTLIB := $(LIB_DIR)/$(LIBRARY).$A

  # `-lm' is required to compile on some Unix systems.
  #
  ifeq ($(PLATFORM),unix)
    MATH := -lm
  endif

  ifeq ($(PLATFORM),unixdev)
    MATH := -lm
  endif

  # The default commands used to link the executables.  These can
  # be redefined for platform-specific stuff.
  #
  ifeq ($(PLATFORM),unix)
    CC   = $(CCraw)
    LINK = $(subst /,$(SEP),$(BUILD_DIR)/libtool) --mode=link $(CC) \
           $T$(subst /,$(COMPILER_SEP),$@ $< $(LDFLAGS) $(FTLIB) $(EFENCE))
  else
    ifeq ($(PLATFORM),unixdev)
      LINK = $(CC) $T$(subst /,$(COMPILER_SEP),$@ $< $(FTLIB) \
                               $(EFENCE) -lm $(LDFLAGS))
    else
      LINK = $(CC) $T$(subst /,$(COMPILER_SEP),$@ $< $(FTLIB) \
                               $(EFENCE) $(LDFLAGS))
    endif
  endif
  
  COMMON_LINK = $(LINK) $(subst /,$(COMPILER_SEP),$(COMMON_OBJ))
  GRAPH_LINK  = $(COMMON_LINK) $(subst /,$(COMPILER_SEP),$(GRAPH_LIB))
  GRAPH_LINK2 = $(GRAPH_LINK) $(subst /,$(COMPILER_SEP),$(EXTRA_GRAPH_OBJS))

  .PHONY: exes clean distclean


  ###################################################################
  #
  # Include the rules needed to compile the graphics sub-system.
  # This will also select which graphics driver to compile to the
  # sub-system.
  #
  include $(GRAPH_DIR)/rules.mk


  ####################################################################
  #
  # Detect DOS-like platforms, currently DOS, Win 3.1, Win32 & OS/2.
  #
  ifneq ($(findstring $(PLATFORM),os2 win16 win32 dos),)
    DOSLIKE := 1
  endif


  ###################################################################
  #
  # Clean-up rules.  Because the `del' command on DOS-like platforms
  # cannot take a long list of arguments, we simply erase the directory
  # contents.
  #
  ifdef DOSLIKE

    clean_demo:
	    -del obj\*.$(SO) 2> nul
	    -del $(subst /,\,$(TOP_DIR_2)/src/*.bak) 2> nul

    distclean_demo: clean_demo
	    -del obj\*.lib 2> nul
	    -del bin\*.exe 2> nul

  else

    clean_demo:
	    -$(DELETE) $(subst /,$(SEP),$(OBJ_DIR)/*.$(SO))
	    -$(DELETE) $(subst /,$(SEP),$(SRC_DIR)/*.bak graph/*.bak)
	    -$(DELETE) $(subst /,$(SEP),$(SRC_DIR)/*~ graph/*~)

    distclean_demo: clean_demo
	    -$(DELETE) $(subst /,$(SEP),$(EXES:%=$(BIN_DIR)/%$E))
	    -$(DELETE) $(subst /,$(SEP),$(GRAPH_LIB))
    ifeq ($(PLATFORM),unix)
	      -$(DELETE) $(BIN_DIR)/.libs/*
	      -$(DELDIR) $(BIN_DIR)/.libs
    endif

  endif

  clean:     clean_demo
  distclean: distclean_demo


  ####################################################################
  #
  # Compute the executable suffix to use, and put it in `E'.
  # It is ".exe" on DOS-ish platforms, and nothing otherwise.
  #
  ifdef DOSLIKE
    E := .exe
  else
    E :=
  endif


  ###################################################################
  #
  # The list of demonstration programs to build.
  #
  EXES := ftlint ftmemchk ftdump testname fttimer ftbench ftchkwd

  # Comment out the next line if you don't have a graphics subsystem.
  EXES += ftview ftmulti ftstring

  # Only uncomment the following lines if the truetype driver was
  # compiled with TT_CONFIG_OPTION_BYTECODE_INTERPRETER defined.
  #
  #  ifneq ($(findstring $(PLATFORM),os2 unix win32),)
  #    EXES += ttdebug
  #  endif

  exes: $(EXES:%=$(BIN_DIR)/%$E)


  INCLUDES := $(subst /,$(COMPILER_SEP),$(FT_INCLUDES))


  ####################################################################
  #
  # Rules for compiling object files for text-only demos.
  #
  COMMON_OBJ := $(OBJ_DIR)/common.$(SO)
  $(COMMON_OBJ): $(SRC_DIR)/common.c
  ifdef DOSLIKE
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<) $DEXPAND_WILDCARDS 
  else
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)
  endif


  $(OBJ_DIR)/%.$(SO): $(SRC_DIR)/%.c $(FTLIB)
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)

  $(OBJ_DIR)/ftlint.$(SO): $(SRC_DIR)/ftlint.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)

  $(OBJ_DIR)/ftbench.$(SO): $(SRC_DIR)/ftbench.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<) $(EXTRAFLAGS)

  $(OBJ_DIR)/ftchkwd.$(SO): $(SRC_DIR)/ftchkwd.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<) $(EXTRAFLAGS)

  $(OBJ_DIR)/compos.$(SO): $(SRC_DIR)/compos.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)

  $(OBJ_DIR)/ftmemchk.$(SO): $(SRC_DIR)/ftmemchk.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)

  $(OBJ_DIR)/fttry.$(SO): $(SRC_DIR)/fttry.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)

  $(OBJ_DIR)/ftdump.$(SO): $(SRC_DIR)/ftdump.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)

  $(OBJ_DIR)/testname.$(SO): $(SRC_DIR)/testname.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)


  $(OBJ_DIR)/ftview.$(SO): $(SRC_DIR)/ftview.c \
                           $(GRAPH_LIB) $(SRC_DIR)/ftcommon.i
	  $(COMPILE) $(GRAPH_INCLUDES:%=$I%) \
                     $T$(subst /,$(COMPILER_SEP),$@ $<) \

  $(OBJ_DIR)/ftmulti.$(SO): $(SRC_DIR)/ftmulti.c \
                            $(GRAPH_LIB) $(SRC_DIR)/ftcommon.i
	  $(COMPILE) $(GRAPH_INCLUDES:%=$I%) \
                     $T$(subst /,$(COMPILER_SEP),$@ $<) \

  $(OBJ_DIR)/ftstring.$(SO): $(SRC_DIR)/ftstring.c \
                             $(GRAPH_LIB) $(SRC_DIR)/ftcommon.i
	  $(COMPILE) $(GRAPH_INCLUDES:%=$I%) \
                     $T$(subst /,$(COMPILER_SEP),$@ $<) \

  $(OBJ_DIR)/fttimer.$(SO): $(SRC_DIR)/fttimer.c $(GRAPH_LIB)
	  $(COMPILE) $(GRAPH_INCLUDES:%=$I%) \
                     $T$(subst /,$(COMPILER_SEP),$@ $<) \


# $(OBJ_DIR)/ftsbit.$(SO): $(SRC_DIR)/ftsbit.c $(GRAPH_LIB)
#	 $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)


  ####################################################################
  #
  # Special rule to compile the `t1dump' program as it includes
  # the Type1 source path.
  #
  $(OBJ_DIR)/t1dump.$(SO): $(SRC_DIR)/t1dump.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)


  ####################################################################
  #
  # Special rule to compile the `ttdebug' program as it includes
  # the TrueType source path and needs extra flags for correct keyboard
  # handling on Unix.

  # POSIX TERMIOS: Do not define if you use OLD U*ix like 4.2BSD.
  #
  # detect a Unix system
  #
  ifeq ($(PLATFORM),unix)
    EXTRAFLAGS = $DUNIX $DHAVE_POSIX_TERMIOS
  endif

  $(OBJ_DIR)/ttdebug.$(SO): $(SRC_DIR)/ttdebug.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<) \
                     $I$(subst /,$(COMPILER_SEP),$(TOP_DIR)/src/truetype) \
                     $(EXTRAFLAGS)


  ####################################################################
  #
  # Rules used to link the executables.  Note that they could be
  # overridden by system-specific things.
  #
  $(BIN_DIR)/ftlint$E: $(OBJ_DIR)/ftlint.$(SO) $(FTLIB) $(COMMON_OBJ)
	  $(COMMON_LINK)

  $(BIN_DIR)/ftbench$E: $(OBJ_DIR)/ftbench.$(SO) $(FTLIB) $(COMMON_OBJ)
	  $(COMMON_LINK)

  $(BIN_DIR)/ftchkwd$E: $(OBJ_DIR)/ftchkwd.$(SO) $(FTLIB) $(COMMON_OBJ)
	  $(COMMON_LINK)

  $(BIN_DIR)/ftmemchk$E: $(OBJ_DIR)/ftmemchk.$(SO) $(FTLIB) $(COMMON_OBJ)
	  $(COMMON_LINK)

  $(BIN_DIR)/compos$E: $(OBJ_DIR)/compos.$(SO) $(FTLIB) $(COMMON_OBJ)
	  $(COMMON_LINK)

  $(BIN_DIR)/ftdump$E: $(OBJ_DIR)/ftdump.$(SO) $(FTLIB)
	  $(COMMON_LINK)

  $(BIN_DIR)/fttry$E: $(OBJ_DIR)/fttry.$(SO) $(FTLIB)
	  $(LINK)

  $(BIN_DIR)/ftsbit$E: $(OBJ_DIR)/ftsbit.$(SO) $(FTLIB)
	  $(LINK)

  $(BIN_DIR)/t1dump$E: $(OBJ_DIR)/t1dump.$(SO) $(FTLIB)
	  $(LINK)

  $(BIN_DIR)/ttdebug$E: $(OBJ_DIR)/ttdebug.$(SO) $(FTLIB)
	  $(LINK)

  $(BIN_DIR)/testname$E: $(OBJ_DIR)/testname.$(SO) $(FTLIB)
	  $(LINK)


  $(BIN_DIR)/ftview$E: $(OBJ_DIR)/ftview.$(SO) $(FTLIB) \
                       $(GRAPH_LIB) $(COMMON_OBJ)
	  $(GRAPH_LINK)

  $(BIN_DIR)/ftmulti$E: $(OBJ_DIR)/ftmulti.$(SO) $(FTLIB) \
                        $(GRAPH_LIB) $(COMMON_OBJ)
	  $(GRAPH_LINK)

  $(BIN_DIR)/ftstring$E: $(OBJ_DIR)/ftstring.$(SO) $(FTLIB) \
                         $(GRAPH_LIB) $(COMMON_OBJ)
	  $(GRAPH_LINK) $(MATH)

  $(BIN_DIR)/fttimer$E: $(OBJ_DIR)/fttimer.$(SO) $(FTLIB) \
                        $(GRAPH_LIB) $(COMMON_OBJ)
	  $(GRAPH_LINK)


endif

# EOF
