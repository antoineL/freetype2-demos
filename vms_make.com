$!---------------vms_make.com for Freetype2 demos ------------------------------
$! make Freetype2 under OpenVMS
$!
$! In case of problems with the build you might want to contact me at
$! zinser@decus.de (preferred) or zinser@sysdev.deutsche-boerse.com (Work)
$!
$!------------------------------------------------------------------------------
$! 
$! Just some general constants
$!
$ Make   = ""
$ true   = 1
$ false  = 0
$!
$! Setup variables holding "config" information
$!
$ name   = "FT2demos"
$ optfile =  name + ".opt"
$ ccopt    = ""
$ lopts    = ""
$!
$! Check for MMK/MMS
$!
$ If F$Search ("Sys$System:MMS.EXE") .nes. "" Then Make = "MMS"
$ If F$Type (MMK) .eqs. "STRING" Then Make = "MMK"
$!
$! Which command parameters were given
$!
$ gosub check_opts
$!
$! Create option file
$!
$ open/write optf 'optfile'
$ If f$getsyi("HW_MODEL") .gt. 1024
$ Then
$  write optf "[-.lib]freetype2shr.exe/share"
$ else
$   write optf "[-.lib]freetype.olb/lib"
$ endif
$ write optf "sys$share:decw$xlibshr.exe/share"
$ close optf
$!
$ gosub crea_mms
$ 'Make'
$ delete/nolog/noconf ccop.mms;*,descrip.fdl;*
$ purge/nolog descrip.mms
$!
$ exit
$!
$!------------------------------------------------------------------------------
$!
$! If MMS/MMK are available dump out the descrip.mms if required 
$!
$CREA_MMS:
$ write sys$output "Creating descrip.mms..."
$ copy sys$input: descrip.mms
$ deck
# This file is part of the FreeType project.
#
# DESCRIP.MMS: Make file for OpenVMS using MMS or MMK
# Created by Martin P.J. Zinser
#    (zinser@decus.de (preferred) or zinser@sysdev.deutsche-boerse.com (work))


.FIRST 

        define freetype [-.include.freetype]
        
CC = cc

# location of src for Test programs
SRCDIR = [.src]
GRAPHSRC = [.graph]
GRX11SRC = [.graph.x11]
OBJDIR = [.obj]

# include paths
INCLUDES = /include=([-.include],[.graph])

GRAPHOBJ = $(OBJDIR)grblit.obj,  \
           $(OBJDIR)grobjs.obj,  \
           $(OBJDIR)grfont.obj,  \
           $(OBJDIR)grinit.obj,  \
           $(OBJDIR)grdevice.obj,\
           $(OBJDIR)grx11.obj

# C flags
CFLAGS = $(CCOPT)$(INCLUDES)/obj=$(OBJDIR)

ALL : ftlint.exe ftmemchk.exe ftdump.exe testnames.exe \
      ftview.exe ftmulti.exe ftstring.exe fttimer.exe 


ftlint.exe    : $(OBJDIR)ftlint.obj
        link $(LOPTS) $(OBJDIR)ftlint.obj,[]ft2demos.opt/opt
ftmemchk.exe  : $(OBJDIR)ftmemchk.obj
        link $(LOPTS) $(OBJDIR)ftmemchk.obj,[]ft2demos.opt/opt
ftdump.exe    : $(OBJDIR)ftdump.obj,$(OBJDIR)common.obj
        link $(LOPTS) $(OBJDIR)ftdump.obj,common.obj,[]ft2demos.opt/opt
testnames.exe : $(OBJDIR)testnames.obj
        link $(LOPTS) $(OBJDIR)testnames.obj,[]ft2demos.opt/opt
ftview.exe    : $(OBJDIR)ftview.obj,$(OBJDIR)common.obj,$(GRAPHOBJ)
        link $(LOPTS) $(OBJDIR)ftview.obj,common.obj,$(GRAPHOBJ),[]ft2demos.opt/opt                              
ftmulti.exe   : $(OBJDIR)ftmulti.obj,$(OBJDIR)common.obj,$(GRAPHOBJ)
        link $(LOPTS) $(OBJDIR)ftmulti.obj,common.obj,$(GRAPHOBJ),[]ft2demos.opt/opt
ftstring.exe  : $(OBJDIR)ftstring.obj,$(OBJDIR)common.obj,$(GRAPHOBJ)
        link $(LOPTS) $(OBJDIR)ftstring.obj,common.obj,$(GRAPHOBJ),[]ft2demos.opt/opt
fttimer.exe   : $(OBJDIR)fttimer.obj
        link $(LOPTS) $(OBJDIR)fttimer.obj,[]ft2demos.opt/opt                    
                
$(OBJDIR)common.obj    : $(SRCDIR)common.c , $(SRCDIR)common.h
$(OBJDIR)ftlint.obj    : $(SRCDIR)ftlint.c
$(OBJDIR)ftmemchk.obj  : $(SRCDIR)ftmemchk.c
$(OBJDIR)ftdump.obj    : $(SRCDIR)ftdump.c
$(OBJDIR)testnames.obj : $(SRCDIR)testnames.c
$(OBJDIR)ftview.obj    : $(SRCDIR)ftview.c
$(OBJDIR)grblit.obj    : $(GRAPHSRC)grblit.c
$(OBJDIR)grobjs.obj    : $(GRAPHSRC)grobjs.c
$(OBJDIR)grfont.obj    : $(GRAPHSRC)grfont.c
$(OBJDIR)grinit.obj    : $(GRAPHSRC)grinit.c
        set def $(GRAPHSRC)
        $(CC)$(CCOPT)/include=([.x11],[])/define=(DEVICE_X11)/obj=[-.obj] grinit.c 
        set def [-]
$(OBJDIR)grx11.obj     : $(GRX11SRC)grx11.c
        set def $(GRX11SRC)
        $(CC)$(CCOPT)/obj=[--.obj]/include=([-]) grx11.c
        set def [--]
$(OBJDIR)grdevice.obj  : $(GRAPHSRC)grdevice.c
$(OBJDIR)ftmulti.obj   : $(SRCDIR)ftmulti.c
$(OBJDIR)ftstring.obj  : $(SRCDIR)ftstring.c
$(OBJDIR)fttimer.obj   : $(SRCDIR)fttimer.c

CLEAN :
       delete $(OBJDIR)*.obj;*,[]ft2demos.opt;*
# EOF
$ eod
$ anal/rms/fdl descrip.mms 
$ open/write mmsf ccop.mms 
$ write mmsf "CCOPT = ", ccopt
$ write mmsf "LOPTS = ", lopts
$ close mmsf
$ convert/fdl=descrip.fdl ccop.mms ccop.mms
$ copy ccop.mms,descrip.mms;-1 descrip.mms 
$ return
$!------------------------------------------------------------------------------
$!
$! Check commandline options and set symbols accordingly
$!
$ CHECK_OPTS:
$ i = 1
$ OPT_LOOP:
$ if i .lt. 9
$ then
$   cparm = f$edit(p'i',"upcase")
$   if cparm .eqs. "DEBUG"
$   then
$     ccopt = ccopt + "/noopt/deb"
$     lopts = lopts + "/deb"
$   endif
$!   if cparm .eqs. "link $(LOPTS)" then link only = true
$   if f$locate("LOPTS",cparm) .lt. f$length(cparm)
$   then
$     start = f$locate("=",cparm) + 1
$     len   = f$length(cparm) - start
$     lopts = lopts + f$extract(start,len,cparm)
$   endif
$   if f$locate("CCOPT",cparm) .lt. f$length(cparm)
$   then
$     start = f$locate("=",cparm) + 1
$     len   = f$length(cparm) - start
$     ccopt = ccopt + f$extract(start,len,cparm)
$   endif
$   i = i + 1
$   goto opt_loop
$ endif
$ return
