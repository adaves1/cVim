### USEDLL  no for statically linked version of run-time, yes for DLL runtime
### BOR	    path to root of Borland C install (c:\bc5)

### (requires cc3250.dll be available in %PATH%)
!if ("$(USEDLL)"=="")
USEDLL = no
!endif

### BOR: root of the BC installation
!if ("$(BOR)"=="")
BOR = c:\bc5
!endif

CC	= $(BOR)\bin\Bcc32
BRC	= $(BOR)\bin\brc32
LINK	= $(BOR)\BIN\ILink32
INCLUDE = $(BOR)\include;.
LIB	= $(BOR)\lib

!if ("$(USEDLL)"=="yes")
RT_DEF = -D_RTLDLL
RT_LIB = cw32i.lib
!else
RT_DEF =
RT_LIB = cw32.lib
!endif


all : cvimext.dll

cvimext.obj : cvimext.cpp cvimext.h
	$(CC) -tWD -I$(INCLUDE) -c -DFEAT_GETTEXT $(RT_DEF) -w- cvimext.cpp

cvimext.res : cvimext.rc
	$(BRC) -r cvimext.rc

cvimext.dll : cvimext.obj cvimext.res
	$(LINK) -L$(LIB) -aa cvimext.obj, cvimext.dll, , c0d32.obj $(RT_LIB) import32.lib, cvimext.def, cvimext.res

clean :
	-@del cvimext.obj
	-@del cvimext.res
	-@del cvimext.dll
