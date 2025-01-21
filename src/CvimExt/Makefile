# Makefile for CvimExt, using MSVC
# Options:
#   DEBUG=yes		Build debug version (for VC7 and maybe later)
#

TARGETOS=BOTh
APPVER=4.0

!if "$(DEBUG)" != "yes"
NODEBUG = 1
!endif

!include <win32.mak>

all: cvimext.dll

cvimext.dll:    cvimext.obj	\
		cvimext.res
# $(implib) /NOLOGO -machine:$(CPU) -def:cvimext.def $** -out:cvimext.lib
# $(link) $(dlllflags) -base:0x1C000000 -out:$*.dll $** $(olelibsdll) shell32.lib cvimext.lib comctl32.lib cvimext.exp
  $(link) $(lflags) -dll -def:cvimext.def -base:0x1C000000 -out:$*.dll $** $(olelibsdll) shell32.lib comctl32.lib

cvimext.obj: cvimext.h

.cpp.obj:
    $(cc) $(cflags) -DFEAT_GETTEXT $(cvarsdll) $*.cpp

cvimext.res: cvimext.rc
    $(rc) $(rcflags) $(rcvars)  cvimext.rc

clean:
    - if exist cvimext.dll del cvimext.dll
    - if exist cvimext.lib del cvimext.lib
    - if exist cvimext.exp del cvimext.exp
    - if exist cvimext.obj del cvimext.obj
    - if exist cvimext.res del cvimext.res
