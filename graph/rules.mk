#**************************************************************************
#*
#*  FreeType demo utilities sub-Makefile
#*
#*  This Makefile is to be included by `freetype/demo/Makefile'.  Its
#*  purpose is to compile MiGS (the Minimalist Graphics Subsystem).
#*
#*  It is written for GNU Make.  Other make utilities are not
#*  supported!
#*
#**************************************************************************


GRAPH_INCLUDES := $(subst /,$(COMPILER_SEP),$(TOP_DIR_2)/graph)
GRAPH_LIB      := $(OBJ_DIR)/graph.$(SA)

GRAPH := $(TOP_DIR_2)/graph

GRAPH_H := $(GRAPH)/graph.h    \
           $(GRAPH)/grevents.h \
           $(GRAPH)/grfont.h   \
           $(GRAPH)/grtypes.h  \
           $(GRAPH)/grobjs.h   \
           $(GRAPH)/grdevice.h \
           $(GRAPH)/grblit.h   \
           $(GRAPH)/gblender.h \
           $(GRAPH)/gblender_blit.h


GRAPH_OBJS := $(OBJ_DIR)/grblit.$(SO)   \
              $(OBJ_DIR)/grobjs.$(SO)   \
              $(OBJ_DIR)/grfont.$(SO)   \
              $(OBJ_DIR)/grdevice.$(SO) \
              $(OBJ_DIR)/grinit.$(SO)   \
              $(OBJ_DIR)/gblender.$(SO) \
              $(OBJ_DIR)/gblender_blit.$(SO)


# Default value for COMPILE_GRAPH_LIB;
# this value can be modified by the system-specific graphics drivers.
#
COMPILE_GRAPH_LIB = ar -r $(subst /,$(COMPILER_SEP),$@ $(GRAPH_OBJS))


# Add the rules used to detect and compile graphics driver depending
# on the current platform.
#
include $(wildcard $(TOP_DIR_2)/graph/*/rules.mk)


#########################################################################
#
# Build the `graph' library from its objects.  This should be changed
# in the future in order to support more systems.  Probably something
# like a `config/<system>' hierarchy with a system-specific rules file
# to indicate how to make a library file, but for now, I'll stick to
# unix, Win32, and OS/2-gcc.
#
#
$(GRAPH_LIB): $(GRAPH_OBJS)
	$(COMPILE_GRAPH_LIB)


# pattern rule for normal sources
#
$(OBJ_DIR)/%.$(SO): $(GRAPH)/%.c $(GRAPH_H)
	$(CC) $(CFLAGS) $(GRAPH_INCLUDES:%=$I%) $T$@ $<


# a special rule is used for 'grinit.o' as it needs the definition
# of some macros like "-DDEVICE_X11" or "-DDEVICE_OS2_PM"
#
$(OBJ_DIR)/grinit.$(SO): $(GRAPH)/grinit.c $(GRAPH_H)
	$(CC) $(CFLAGS) $(GRAPH_INCLUDES:%=$I%) \
              $(DEVICES:%=$DDEVICE_%) $T$(subst /,$(COMPILER_SEP),$@ $<)


# EOF
